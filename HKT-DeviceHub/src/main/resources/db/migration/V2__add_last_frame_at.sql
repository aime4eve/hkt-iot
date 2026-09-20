-- Catch-up lag semantics: last_frame_at tracks the newest frame the device
-- has on the TB side, so health lag = max(last_frame_at - cursor, 0) instead
-- of now - cursor (silent low-frequency devices no longer look "stuck").
ALTER TABLE registered_devices ADD COLUMN last_frame_at TIMESTAMPTZ;
