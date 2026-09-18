CREATE TABLE registered_devices (
    id                   BIGSERIAL PRIMARY KEY,
    dev_eui              VARCHAR(64) NOT NULL,
    tb_device_id         UUID,
    project              VARCHAR(32) NOT NULL CHECK (project IN ('LIVESTOCK', 'PARKING')),
    external_ref         VARCHAR(128),
    capabilities         JSONB NOT NULL DEFAULT '{}',
    telemetry_cursor_ms  BIGINT,
    last_event_at        TIMESTAMPTZ,
    consecutive_failures INT NOT NULL DEFAULT 0,
    status               VARCHAR(16) NOT NULL DEFAULT 'PENDING',
    created_at           TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at           TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT uk_registered_devices_project_eui UNIQUE (project, dev_eui)
);

CREATE INDEX idx_registered_devices_status ON registered_devices (status);
CREATE INDEX idx_registered_devices_project ON registered_devices (project);
