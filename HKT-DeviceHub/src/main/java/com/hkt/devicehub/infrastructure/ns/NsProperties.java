package com.hkt.devicehub.infrastructure.ns;

import jakarta.annotation.PostConstruct;
import lombok.Getter;
import lombok.Setter;
import org.springframework.boot.context.properties.ConfigurationProperties;
import org.springframework.stereotype.Component;

/**
 * ChirpStack-compatible NS connection settings (devicehub.ns.*). Credentials
 * come from environment variables, never hard-coded.
 */
@Component
@ConfigurationProperties(prefix = "devicehub.ns")
@Getter
@Setter
public class NsProperties {

    private boolean enabled = false;
    private String baseUrl = "http://172.17.201.15:8080";
    private String username = "";
    private String password = "";
    private int orgId = 1;
    private int pageSize = 100;

    @PostConstruct
    void validateCredentials() {
        if (enabled && (username == null || username.isBlank()
                || password == null || password.isBlank())) {
            throw new IllegalStateException(
                    "devicehub.ns.enabled=true requires username and password");
        }
    }
}
