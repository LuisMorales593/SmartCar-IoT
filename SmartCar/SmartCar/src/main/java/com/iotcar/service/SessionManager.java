package com.iotcar.service;

import jakarta.servlet.http.HttpSession;
import org.springframework.stereotype.Component;

import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

@Component
public class SessionManager {
    private static final Map<Long, HttpSession> sesionesPorVehiculo = new ConcurrentHashMap<>();

    public void registrarSesion(Long vehiculoId, HttpSession session) {
        sesionesPorVehiculo.put(vehiculoId, session);
    }

    public void limpiarSesion(Long vehiculoId) {
        HttpSession session = sesionesPorVehiculo.remove(vehiculoId);
        if (session != null) {
            session.setAttribute("velocidadActual", 0);
        }
    }
}