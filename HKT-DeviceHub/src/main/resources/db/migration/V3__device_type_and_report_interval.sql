-- R-06/R-11: report-interval awareness + queryable device type.
-- expected_report_interval_seconds default 3600 (geomagnetic); registration
-- overwrites per deviceType (capsule 14400, tracker 60); self-learning TODO.
ALTER TABLE registered_devices
    ADD COLUMN device_type VARCHAR(32),
    ADD COLUMN expected_report_interval_seconds INT NOT NULL DEFAULT 3600;

CREATE INDEX idx_registered_devices_dev_eui ON registered_devices (dev_eui);
