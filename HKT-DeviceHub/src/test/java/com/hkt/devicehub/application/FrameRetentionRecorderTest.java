package com.hkt.devicehub.application;

import com.hkt.devicehub.domain.repository.TelemetryFrameRecordRepository;
import com.hkt.devicehub.infrastructure.config.QualityProperties;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;

import java.time.Instant;
import java.util.Map;
import java.util.UUID;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

/**
 * Retention contract: offer() never throws and is O(1); flush collapses
 * duplicates via ON CONFLICT (insert returning 0 counts as duplicate); a
 * failed flush re-queues the drained batch instead of dropping it. The
 * recorder must not depend on any entity (2026-11-02 stuck-cursor lesson).
 */
class FrameRetentionRecorderTest {

    private TelemetryFrameRecordRepository repository;
    private QualityProperties properties;
    private FrameRetentionRecorder recorder;

    @BeforeEach
    void setUp() {
        repository = mock(TelemetryFrameRecordRepository.class);
        properties = new QualityProperties();
        recorder = new FrameRetentionRecorder(repository, properties);
    }

    private TelemetryFrame frame(String frameId) {
        return new TelemetryFrame(1, frameId, "001a0103ff000319", "tb-1",
                1_700_000_000_000L, Map.of(), "DEADBEEF", -90, 5, "gw-1");
    }

    @Test
    void offerThenFlushPersistsAndCountsInserts() {
        when(repository.insertIgnoreConflict(any(), anyString(), anyString(), anyString(),
                any(Instant.class), any(), any(), any(), any())).thenReturn(1);

        recorder.offer("LIVESTOCK", frame("11111111-1111-1111-1111-111111111111"));
        recorder.offer("PARKING", frame("22222222-2222-2222-2222-222222222222"));
        recorder.flush();

        FrameRetentionRecorder.Status status = recorder.status();
        assertEquals(2, status.insertedFrames());
        assertEquals(0, status.pending());
        assertEquals(0, status.duplicateFrames());
        verify(repository).insertIgnoreConflict(UUID.fromString("22222222-2222-2222-2222-222222222222"),
                "PARKING", "001a0103ff000319", "tb-1", Instant.ofEpochMilli(1_700_000_000_000L),
                "DEADBEEF", -90, 5, "gw-1");
    }

    @Test
    void zeroInsertCountsAsDuplicate() {
        when(repository.insertIgnoreConflict(any(), anyString(), anyString(), anyString(),
                any(Instant.class), any(), any(), any(), any())).thenReturn(0);

        recorder.offer("LIVESTOCK", frame("11111111-1111-1111-1111-111111111111"));
        recorder.flush();

        assertEquals(1, recorder.status().duplicateFrames());
        assertEquals(0, recorder.status().insertedFrames());
    }

    @Test
    void flushFailureRequeuesInsteadOfDropping() {
        when(repository.insertIgnoreConflict(any(), anyString(), anyString(), anyString(),
                any(Instant.class), any(), any(), any(), any()))
                .thenThrow(new IllegalStateException("db down"));

        recorder.offer("LIVESTOCK", frame("11111111-1111-1111-1111-111111111111"));
        recorder.flush();

        FrameRetentionRecorder.Status afterFailure = recorder.status();
        assertEquals(1, afterFailure.pending());
        assertNotNull(afterFailure.lastError());

        // DB recovers: the re-queued frame is retried on the next flush.
        when(repository.insertIgnoreConflict(any(), anyString(), anyString(), anyString(),
                any(Instant.class), any(), any(), any(), any())).thenReturn(1);
        recorder.flush();
        assertEquals(0, recorder.status().pending());
        assertEquals(1, recorder.status().insertedFrames());
    }

    @Test
    void disabledRetentionNeverTouchesRepository() {
        properties.setFrameRetentionEnabled(false);

        recorder.offer("LIVESTOCK", frame("11111111-1111-1111-1111-111111111111"));
        recorder.flush();

        verify(repository, never()).insertIgnoreConflict(any(), anyString(), anyString(), anyString(),
                any(Instant.class), any(), any(), any(), any());
    }

    @Test
    void overflowDropsOldestAndCountsIt() {
        properties.setMaxPendingFrames(2);
        when(repository.insertIgnoreConflict(any(), anyString(), anyString(), anyString(),
                any(Instant.class), any(), any(), any(), any())).thenReturn(1);

        recorder.offer("LIVESTOCK", frame("11111111-1111-1111-1111-111111111111"));
        recorder.offer("LIVESTOCK", frame("22222222-2222-2222-2222-222222222222"));
        recorder.offer("LIVESTOCK", frame("33333333-3333-3333-3333-333333333333"));
        recorder.flush();

        ArgumentCaptor<UUID> ids = ArgumentCaptor.forClass(UUID.class);
        verify(repository, org.mockito.Mockito.times(2))
                .insertIgnoreConflict(ids.capture(), anyString(), anyString(), anyString(),
                        any(Instant.class), any(), any(), any(), any());
        // Oldest frame dropped, newest two survive.
        assertEquals(UUID.fromString("22222222-2222-2222-2222-222222222222"), ids.getAllValues().get(0));
        assertEquals(UUID.fromString("33333333-3333-3333-3333-333333333333"), ids.getAllValues().get(1));
        assertEquals(1, recorder.status().droppedFrames());
    }

    @Test
    void malformedFrameIdNeverThrowsOutOfOffer() {
        recorder.offer("LIVESTOCK", frame("not-a-uuid"));
        assertEquals(0, recorder.status().pending());
        assertTrue(recorder.status().insertedFrames() == 0);
    }
}
