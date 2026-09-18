package com.hkt.devicehub.application;

import com.fasterxml.jackson.annotation.JsonInclude;

import java.util.Map;

/**
 * Normalized telemetry frame — the single event contract DeviceHub publishes
 * to RocketMQ topic DEVICEHUB_TELEMETRY_FRAME (tag = project).
 * <p>
 * frameId is a stable UUID derived from tbDeviceId + ts (UUID name-based,
 * SHA-1), so consumers can dedup retries, backfill overlaps and WS/REST
 * double delivery idempotently. See docs/telemetry-event-contract.md.
 *
 * @param version    contract version, currently always 1
 * @param frameId    stable dedup key, UUID name-based from tbDeviceId:ts
 * @param devEui     LoRaWAN DevEUI (lower-case hex)
 * @param tbDeviceId ThingsBoard device UUID (string form)
 * @param ts         frame timestamp in epoch millis (TB timeseries ts)
 * @param properties decoded payload properties (authoritative result frames)
 * @param dataHex    raw uplink hex, present when no authoritative result exists
 * @param rssi       transport metadata, may be null
 * @param snr        transport metadata, may be null
 * @param gatewayId  downlink/uplink gateway id reported by TB, may be null
 */
@JsonInclude(JsonInclude.Include.NON_NULL)
public record TelemetryFrame(
        int version,
        String frameId,
        String devEui,
        String tbDeviceId,
        long ts,
        Map<String, Object> properties,
        String dataHex,
        Integer rssi,
        Integer snr,
        String gatewayId) {

    public static final int CONTRACT_VERSION = 1;
}
