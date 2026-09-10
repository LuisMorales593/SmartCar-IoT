package com.iotcar.service;

import com.iotcar.entity.Vehiculo;
import com.iotcar.repository.VehiculoRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.scheduling.annotation.Scheduled;
import org.springframework.stereotype.Service;

import java.time.LocalDateTime;
import java.util.List;

@Service
public class VehiculoEstadoServicio {

    @Autowired
    private VehiculoRepository vehiculoRepository;

    @Autowired
    private SessionManager sessionManager;

    @Scheduled(fixedDelay = 10000)
    public void actualizarEstadoDesconectados() {
        LocalDateTime limite = LocalDateTime.now().minusSeconds(10);
        List<Vehiculo> vehiculos = vehiculoRepository.findAll();

        for (Vehiculo v : vehiculos) {
            if ("CONECTADO".equals(v.getEstado()) &&
                (v.getUltimaConexion() == null || v.getUltimaConexion().isBefore(limite))) {
                v.setEstado("DESCONECTADO");
                vehiculoRepository.save(v);
                // Limpiar velocidad de la sesión asociada a este vehículo
                sessionManager.limpiarSesion(v.getId());
                System.out.println("Vehículo " + v.getId() + " marcado como DESCONECTADO. Velocidad reseteada a 0.");
            }
        }
    }
}