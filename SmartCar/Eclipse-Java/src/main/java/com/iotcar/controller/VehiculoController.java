package com.iotcar.controller;

import com.iotcar.entity.Vehiculo;
import com.iotcar.repository.VehiculoRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpStatus;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.time.LocalDateTime;
import java.util.List;
import java.util.Optional;

@RestController
@RequestMapping("/vehiculos")
@CrossOrigin(origins = "*")
public class VehiculoController {

    @Autowired
    private VehiculoRepository repository;

    @GetMapping
    public List<Vehiculo> listar() {
        return repository.findAll();
    }

    @GetMapping("/{id}")
    public ResponseEntity<Vehiculo> obtener(@PathVariable Long id) {
        Optional<Vehiculo> vehiculo = repository.findById(id);
        return vehiculo.map(ResponseEntity::ok)
                       .orElse(ResponseEntity.notFound().build());
    }

    @PostMapping
    public ResponseEntity<Vehiculo> crear(@RequestBody Vehiculo vehiculo) {
        if (vehiculo.getUltimaConexion() == null) {
            vehiculo.setUltimaConexion(LocalDateTime.now());
        }
        Vehiculo guardado = repository.save(vehiculo);
        return ResponseEntity.status(HttpStatus.CREATED).body(guardado);
    }

    @PutMapping("/{id}")
    public ResponseEntity<Vehiculo> actualizar(@PathVariable Long id, @RequestBody Vehiculo vehiculo) {
        Optional<Vehiculo> optional = repository.findById(id);
        if (!optional.isPresent()) {
            return ResponseEntity.notFound().build();
        }

        Vehiculo existente = optional.get();

        // Actualizar solo los campos que NO son null
        if (vehiculo.getEstado() != null) {
            existente.setEstado(vehiculo.getEstado());
        }
        if (vehiculo.getIp() != null) {
            existente.setIp(vehiculo.getIp());
        }
        if (vehiculo.getNombre() != null) {
            existente.setNombre(vehiculo.getNombre());
        }
        if (vehiculo.getModelo() != null) {
            existente.setModelo(vehiculo.getModelo());
        }
        if (vehiculo.getControlador() != null) {
            existente.setControlador(vehiculo.getControlador());
        }
        if (vehiculo.getUrlStream() != null) {
            existente.setUrlStream(vehiculo.getUrlStream());
        }

        // Siempre actualizar la fecha de conexión
        existente.setUltimaConexion(LocalDateTime.now());

        Vehiculo actualizado = repository.save(existente);
        return ResponseEntity.ok(actualizado);
    }

    @DeleteMapping("/{id}")
    public ResponseEntity<Void> eliminar(@PathVariable Long id) {
        if (!repository.existsById(id)) {
            return ResponseEntity.notFound().build();
        }
        repository.deleteById(id);
        return ResponseEntity.noContent().build();
    }
}