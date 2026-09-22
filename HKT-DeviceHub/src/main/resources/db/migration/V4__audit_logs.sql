-- R-04: governance actions (e.g. deleting empty TB copies) must leave an
-- audit trail. operator comes from the X-Operator request header.
CREATE TABLE audit_logs (
    id          BIGSERIAL PRIMARY KEY,
    action      VARCHAR(64)  NOT NULL,
    target      VARCHAR(128) NOT NULL,
    operator    VARCHAR(64)  NOT NULL DEFAULT 'console',
    detail      JSONB        NOT NULL DEFAULT '{}',
    created_at  TIMESTAMPTZ  NOT NULL DEFAULT now()
);

CREATE INDEX idx_audit_logs_target ON audit_logs (target);
CREATE INDEX idx_audit_logs_created_at ON audit_logs (created_at);
