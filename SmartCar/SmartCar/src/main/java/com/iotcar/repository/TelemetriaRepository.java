package com.iotcar.repository;

import org.springframework.data.jpa.repository.JpaRepository;
import com.iotcar.entity.Telemetria;

public interface TelemetriaRepository extends JpaRepository<Telemetria, Long> {

}