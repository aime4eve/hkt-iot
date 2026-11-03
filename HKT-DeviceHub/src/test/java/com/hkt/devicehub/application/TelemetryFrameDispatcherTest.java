package com.hkt.devicehub.application;

import com.hkt.devicehub.application.disposal.DisposalService;
import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.infrastructure.monitoring.TelemetryChannelMetrics;
import com.hkt.devicehub.infrastructure.mq.TelemetryEventPublisher;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;

import java.time.Instant;
import java.util.Map;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

/**
 * The dispatcher must publish and record metrics but never persist the
 * passed entity: WS callers hand in a long-lived subscription-cached
 * RegisteredDevice whose stale telemetry_cursor_ms must not reach the DB
 * (2026-11-02 stuck-cursor incident). The compile-time guarantee is that the
 * dispatcher holds no repository at all; these tests pin the behavioural
 * contract (publish result, hook, metrics, fail-open).
 */
class TelemetryFrameDispatcherTest {

    private TelemetryEventPublisher publisher;
    private TelemetryChannelMetrics metrics;
    private DisposalService disposalService;
    private FrameRetentionRecorder frameRetentionRecorder;
    private TelemetryFrameDispatcher dispatcher;
    private RegisteredDevice device;
    private TelemetryFrame frame;

    @BeforeEach
    void setUp() {
        publisher = mock(TelemetryEventPublisher.class);
        metrics = mock(TelemetryChannelMetrics.class);
        disposalService = mock(DisposalService.class);
        frameRetentionRecorder = mock(FrameRetentionRecorder.class);
        dispatcher = new TelemetryFrameDispatcher(publisher, metrics, disposalService, frameRetentionRecorder);

        device = new RegisteredDevice();
        device.setDevEui("00956906001264ef");
        device.setProject(DeviceProject.LIVESTOCK);
        frame = new TelemetryFrame(1, "f-1", device.getDevEui(), "tb-1",
                1_700_000_000_000L, Map.of(), "DEADBEEF", -90, 10, "gw-1");
    }

    @Test
    void publishedFrameReturnsTrueAndTriggersDisposalHook() {
        when(publisher.publish(any(TelemetryFrame.class), anyString())).thenReturn(true);

        boolean sent = dispatcher.dispatch(device, frame, TelemetryChannelMetrics.CHANNEL_WS);

        assertTrue(sent);
        verify(frameRetentionRecorder).offer("LIVESTOCK", frame);
        verify(publisher).publish(frame, "LIVESTOCK");
        ArgumentCaptor<Instant> ts = ArgumentCaptor.forClass(Instant.class);
        verify(disposalService).onFrameArrived(eq(device.getDevEui()), ts.capture());
        assertEquals(Instant.ofEpochMilli(frame.ts()), ts.getValue());
        verify(metrics).recordFrame(TelemetryChannelMetrics.CHANNEL_WS, "LIVESTOCK", "published");
        verify(metrics).recordPushLatency(TelemetryChannelMetrics.CHANNEL_WS, frame.ts());
    }

    @Test
    void failedPublishReturnsFalseAndSkipsDisposalHook() {
        when(publisher.publish(any(TelemetryFrame.class), anyString())).thenReturn(false);

        boolean sent = dispatcher.dispatch(device, frame, TelemetryChannelMetrics.CHANNEL_REST);

        assertFalse(sent);
        // Retention happens regardless of publish outcome — evidence first.
        verify(frameRetentionRecorder).offer("LIVESTOCK", frame);
        verify(disposalService, never()).onFrameArrived(anyString(), any(Instant.class));
        verify(metrics).recordFrame(TelemetryChannelMetrics.CHANNEL_REST, "LIVESTOCK", "mq_failed");
    }

    @Test
    void disposalHookFailureNeverFailsTheFrame() {
        when(publisher.publish(any(TelemetryFrame.class), anyString())).thenReturn(true);
        doThrow(new RuntimeException("db down"))
                .when(disposalService).onFrameArrived(anyString(), any(Instant.class));

        boolean sent = dispatcher.dispatch(device, frame, TelemetryChannelMetrics.CHANNEL_WS);

        assertTrue(sent);
        verify(metrics).recordPushLatency(anyString(), anyLong());
    }
}
