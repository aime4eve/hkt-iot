package com.hkt.devicehub.infrastructure.thingsboard;

import jakarta.annotation.PostConstruct;
import lombok.Getter;
import lombok.Setter;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

/**
 * ThingsBoard channel configuration (devicehub.tb.*). Credentials come from
 * environment variables via application.yml placeholders, never hard-coded.
 */
@Component
@ConfigurationProperties(prefix = "devicehub.tb")
@Getter
@Setter
public class TbProperties {

    private boolean enabled = false;
    private boolean wsEnabled = true;
    private String baseUrl = "http://172.22.3.105";
    private String username = "tenant@hkt.com";
    private String password = "";
    private long wsPingMs = 30_000;
    private long pollIntervalMs = 300_000;
    private int lookbackDays = 7;
    private int batchSize = 200;

    @PostConstruct
    void validateCredentials() {
        if (enabled && (username == null || username.isBlank()
                || password == null || password.isBlank())) {
            throw new IllegalStateException(
                    "devicehub.tb.enabled=true requires username and password");
        }
    }
}
