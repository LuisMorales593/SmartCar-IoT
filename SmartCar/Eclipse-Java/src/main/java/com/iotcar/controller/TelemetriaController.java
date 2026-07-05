package com.iotcar.controller;

import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import com.iotcar.entity.Telemetria;
import com.iotcar.repository.TelemetriaRepository;

@RestController
@RequestMapping("/telemetria")
public class TelemetriaController {

    @Autowired
    private TelemetriaRepository telemetriaRepository;

    @GetMapping
    public List<Telemetria> listarTelemetria() {
        return telemetriaRepository.findAll();
    }
}