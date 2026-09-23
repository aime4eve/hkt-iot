package com.hkt.devicehub.interfaces;

import org.springframework.context.annotation.Configuration;
import org.springframework.web.servlet.config.annotation.ViewControllerRegistry;
import org.springframework.web.servlet.config.annotation.WebMvcConfigurer;

/**
 * Spring Boot 3 serves welcome pages only at the root; forward /console
 * variants to the console single-page app explicitly.
 */
@Configuration
public class ConsoleWebConfig implements WebMvcConfigurer {

    @Override
    public void addViewControllers(ViewControllerRegistry registry) {
        registry.addViewController("/console")
                .setViewName("forward:/console/index.html");
        registry.addViewController("/console/")
                .setViewName("forward:/console/index.html");
    }
}
