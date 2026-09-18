package com.hkt.devicehub;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.scheduling.annotation.EnableScheduling;

/**
 * HKT-DeviceHub — shared device provisioning + telemetry ingestion hub in
 * front of ThingsBoard, publishing normalized frames to RocketMQ.
 */
@SpringBootApplication
@EnableScheduling
public class DeviceHubApplication {

    public static void main(String[] args) {
        SpringApplication.run(DeviceHubApplication.class, args);
    }
}
