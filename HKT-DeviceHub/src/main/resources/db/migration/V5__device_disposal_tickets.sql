-- Console v1.5 §3.4: no-data device disposal queue (workflow, not a list).
CREATE TABLE device_disposal_tickets (
    id                    BIGSERIAL PRIMARY KEY,
    dev_eui               VARCHAR(64)  NOT NULL,
    project               VARCHAR(32)  NOT NULL CHECK (project IN ('LIVESTOCK', 'PARKING')),
    severity              VARCHAR(16)  NOT NULL DEFAULT 'NOTICE'
                          CHECK (severity IN ('NOTICE', 'CRITICAL')),
    category              VARCHAR(16)  CHECK (category IN ('FIELD', 'PLATFORM')),
    triage_evidence       JSONB        NOT NULL DEFAULT '{}',
    status                VARCHAR(24)  NOT NULL DEFAULT 'PENDING_ASSIGN'
                          CHECK (status IN ('PENDING_ASSIGN', 'APPROVING', 'PROCESSING',
                                            'OBSERVING', 'RESOLVED', 'DECOMMISSIONED')),
    assignee              VARCHAR(64),
    dingtalk_instance_id  VARCHAR(64),
    decommission_reason   VARCHAR(512),
    first_frame_at        TIMESTAMPTZ,
    timeline              JSONB        NOT NULL DEFAULT '[]',
    created_at            TIMESTAMPTZ  NOT NULL DEFAULT now(),
    updated_at            TIMESTAMPTZ  NOT NULL DEFAULT now()
);

CREATE INDEX idx_disposal_tickets_dev_eui ON device_disposal_tickets (dev_eui);
CREATE INDEX idx_disposal_tickets_status ON device_disposal_tickets (status);
