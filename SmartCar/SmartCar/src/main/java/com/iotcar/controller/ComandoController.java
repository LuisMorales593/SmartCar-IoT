package com.iotcar.controller;

import java.util.List;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import com.iotcar.entity.Comando;
import com.iotcar.repository.ComandoRepository;

@RestController
@RequestMapping("/comandos")
public class ComandoController {

    @Autowired
    private ComandoRepository comandoRepository;

    // GET /comandos → listar todos
    @GetMapping
    public List<Comando> listarComandos() {
        return comandoRepository.findAll();
    }

    // GET /comandos/{vehiculoId} → listar por vehículo
    @GetMapping("/{vehiculoId}")
    public List<Comando> listarPorVehiculo(@PathVariable Long vehiculoId) {
        return comandoRepository.findByVehiculoId(vehiculoId);
    }
}