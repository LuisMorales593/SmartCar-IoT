package com.iotcar.controller;

import com.iotcar.entity.Sesion;
import com.iotcar.repository.SesionRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.time.LocalDateTime;
import java.util.List;

@RestController
@RequestMapping("/sesiones")
public class SesionController {

    @Autowired
    private SesionRepository sesionRepository;

    // Listar todas las sesiones
    @GetMapping
    public List<Sesion> listar() {
        return sesionRepository.findAll();
    }

    // Obtener sesión activa de un usuario
    @GetMapping("/usuario/{usuarioId}/activa")
    public Sesion sesionActivaPorUsuario(@PathVariable Long usuarioId) {
        return sesionRepository.findFirstByUsuarioIdAndActivaTrueOrderByFechaInicioDesc(usuarioId).orElse(null);
    }

    // Obtener sesiones activas de un vehículo
    @GetMapping("/vehiculo/{vehiculoId}/activas")
    public List<Sesion> sesionesActivasPorVehiculo(@PathVariable Long vehiculoId) {
        return sesionRepository.findByVehiculoIdAndActivaTrue(vehiculoId);
    }

    // Cerrar sesión (marcar como inactiva)
    @PutMapping("/cerrar/{id}")
    public String cerrarSesion(@PathVariable Long id) {
        Sesion sesion = sesionRepository.findById(id).orElse(null);
        if (sesion != null && sesion.getActiva()) {
            sesion.setActiva(false);
            sesion.setFechaFin(LocalDateTime.now());
            sesionRepository.save(sesion);
            return "Sesión cerrada correctamente";
        }
        return "Sesión no encontrada o ya cerrada";
    }

    // Eliminar sesión (físicamente)
    @DeleteMapping("/{id}")
    public String eliminar(@PathVariable Long id) {
        if (sesionRepository.existsById(id)) {
            sesionRepository.deleteById(id);
            return "Sesión eliminada";
        }
        return "Sesión no encontrada";
    }
}