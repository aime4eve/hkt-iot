package com.hkt.devicehub.domain.repository;

import com.hkt.devicehub.domain.model.TelemetryFrameRecord;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Modifying;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.transaction.annotation.Transactional;

import java.time.Instant;
import java.util.List;
import java.util.UUID;

public interface TelemetryFrameRecordRepository extends JpaRepository<TelemetryFrameRecord, Long> {

    /** Conflict-free insert: WS/REST/backup double delivery collapses to one
     * row. Returns 1 when inserted, 0 when the frameId already exists. Runs in
     * its own transaction so one bad row never drags the whole flush batch. */
    @Transactional
    @Modifying
    @Query(value = """
            INSERT INTO telemetry_frame_records
                (frame_id, project, dev_eui, tb_device_id, ts, data_hex, rssi, snr, gateway_id, received_at)
            VALUES
                (:frameId, :project, :devEui, :tbDeviceId, :ts, :dataHex, :rssi, :snr, :gatewayId, now())
            ON CONFLICT (frame_id) DO NOTHING
            """, nativeQuery = true)
    int insertIgnoreConflict(@Param("frameId") UUID frameId,
                             @Param("project") String project,
                             @Param("devEui") String devEui,
                             @Param("tbDeviceId") String tbDeviceId,
                             @Param("ts") Instant ts,
                             @Param("dataHex") String dataHex,
                             @Param("rssi") Integer rssi,
                             @Param("snr") Integer snr,
                             @Param("gatewayId") String gatewayId);

    List<TelemetryFrameRecord> findByTsGreaterThanEqualOrderByTsAsc(Instant since);
}
