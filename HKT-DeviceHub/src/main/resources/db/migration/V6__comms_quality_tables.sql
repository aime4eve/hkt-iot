-- Communication quality MVP (research note 2026-11-03): per-frame retention
-- + NS counter snapshots. DeviceHub previously persisted nothing about frames
-- (MQ-only); this makes loss-rate / signal-distribution / gap metrics
-- computable locally. frame_id is the stable dedup key shared with the MQ
-- contract, so WS/REST/backup-channel double delivery collapses to one row.
CREATE TABLE telemetry_frame_records (
    id           BIGSERIAL PRIMARY KEY,
    frame_id     UUID         NOT NULL,
    project      VARCHAR(32)  NOT NULL,
    dev_eui      VARCHAR(64)  NOT NULL,
    tb_device_id VARCHAR(64),
    ts           TIMESTAMPTZ  NOT NULL,
    data_hex     TEXT,
    rssi         INTEGER,
    snr          INTEGER,
    gateway_id   VARCHAR(128),
    received_at  TIMESTAMPTZ  NOT NULL DEFAULT now()
);

CREATE UNIQUE INDEX uk_telemetry_frame_records_frame_id
    ON telemetry_frame_records (frame_id);
CREATE INDEX idx_telemetry_frame_records_dev_ts
    ON telemetry_frame_records (dev_eui, ts DESC);
CREATE INDEX idx_telemetry_frame_records_project_ts
    ON telemetry_frame_records (project, ts DESC);

-- One row per device per poll cycle. ns_ok=false rows are evidence of NS
-- unavailability (unknown != healthy), never silently skipped.
CREATE TABLE ns_counter_snapshots (
    id             BIGSERIAL PRIMARY KEY,
    dev_eui        VARCHAR(64) NOT NULL,
    f_cnt_up       BIGINT,
    n_f_cnt_down   BIGINT,
    a_f_cnt_down   BIGINT,
    latest_gateway VARCHAR(128),
    last_seen      TIMESTAMPTZ,
    ns_ok          BOOLEAN     NOT NULL DEFAULT FALSE,
    error          VARCHAR(256),
    polled_at      TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX idx_ns_counter_snapshots_dev_polled
    ON ns_counter_snapshots (dev_eui, polled_at DESC);
