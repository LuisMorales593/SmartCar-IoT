package com.iotcar;

import org.springframework.boot.SpringApplication;
import org.springframework.boot.autoconfigure.SpringBootApplication;
import org.springframework.scheduling.annotation.EnableScheduling;

@SpringBootApplication
@EnableScheduling
public class IotcarApplication {
    public static void main(String[] args) {
        SpringApplication.run(IotcarApplication.class, args);
    }
}