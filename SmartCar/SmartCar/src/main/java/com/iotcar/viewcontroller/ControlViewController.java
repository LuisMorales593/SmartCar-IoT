package com.iotcar.viewcontroller;

import com.iotcar.entity.Comando;
import com.iotcar.entity.Sesion;
import com.iotcar.entity.Vehiculo;
import com.iotcar.repository.ComandoRepository;
import com.iotcar.repository.SesionRepository;
import com.iotcar.repository.VehiculoRepository;
import com.iotcar.config.ComandoWebSocketHandler;
import jakarta.servlet.http.HttpSession;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestParam;

import java.time.LocalDateTime;
import java.util.List;

@Controller
public class ControlViewController {

    private static final String VELOCIDAD_SESSION_KEY = "velocidadActual";
    private static final String MOTOR_SESSION_KEY = "motorEncendido";
    private static final String SISTEMA_SESSION_KEY = "sistemaEncendido";

    @Autowired
    private ComandoRepository comandoRepository;

    @Autowired
    private VehiculoRepository vehiculoRepository;

    @Autowired
    private SesionRepository sesionRepository;

    @GetMapping("/web/control")
    public String control(@RequestParam(required = false) Long vehiculoId,
                          HttpSession session,
                          Model model) {
        if (vehiculoId == null) {
            vehiculoId = (Long) session.getAttribute("vehiculoId");
        }

        model.addAttribute("contenido", "control");
        model.addAttribute("vehiculoId", vehiculoId);

        Integer velocidadActual = (Integer) session.getAttribute(VELOCIDAD_SESSION_KEY);
        if (velocidadActual == null) velocidadActual = 0;
        model.addAttribute("velocidadActual", velocidadActual);

        Boolean motorEncendido = (Boolean) session.getAttribute(MOTOR_SESSION_KEY);
        if (motorEncendido == null) motorEncendido = false;
        model.addAttribute("motorEncendido", motorEncendido);

        Boolean sistemaEncendido = (Boolean) session.getAttribute(SISTEMA_SESSION_KEY);
        if (sistemaEncendido == null) sistemaEncendido = false;
        model.addAttribute("sistemaEncendido", sistemaEncendido);

        if (vehiculoId != null) {
            Vehiculo vehiculo = vehiculoRepository.findById(vehiculoId).orElse(null);
            if (vehiculo != null) {
                model.addAttribute("vehiculoConectado", "CONECTADO".equals(vehiculo.getEstado()));
                List<Sesion> sesiones = sesionRepository.findByVehiculoIdAndActivaTrue(vehiculoId);
                if (!sesiones.isEmpty()) {
                    model.addAttribute("sesionId", sesiones.get(0).getId());
                }
            } else {
                model.addAttribute("vehiculoConectado", false);
            }
        } else {
            model.addAttribute("vehiculoConectado", false);
        }

        return "layout";
    }

    @PostMapping("/web/control/comando")
    public String recibirComando(@RequestParam String comando,
                                 @RequestParam Long vehiculoId,
                                 @RequestParam(required = false) String valor,
                                 HttpSession session) {

        Long usuarioId = (Long) session.getAttribute("usuarioId");
        if (usuarioId == null) usuarioId = 1L;

        String mensajeWS;

        // ===== SISTEMA (nivel 1) =====
        if ("arrancar".equals(comando)) {
            boolean encendido = "on".equals(valor);
            session.setAttribute(SISTEMA_SESSION_KEY, encendido);
            if (!encendido) {
                session.setAttribute(VELOCIDAD_SESSION_KEY, 0);
                session.setAttribute(MOTOR_SESSION_KEY, false);
            }
            mensajeWS = "{\"tipo\":\"arrancar\",\"valor\":\"" + (encendido ? "on" : "off") + "\"}";
        }
        // ===== MOTOR (nivel 2) =====
        else if ("motor".equals(comando)) {
            boolean encendido = "on".equals(valor);
            session.setAttribute(MOTOR_SESSION_KEY, encendido);
            if (!encendido) {
                session.setAttribute(VELOCIDAD_SESSION_KEY, 0);
            }
            mensajeWS = "{\"tipo\":\"motor\",\"valor\":\"" + (encendido ? "on" : "off") + "\"}";
        }
        // ===== VELOCIDAD =====
        else if ("velocidad".equals(comando)) {
            int v = Integer.parseInt(valor);
            if (v < -100) v = -100;
            if (v > 100) v = 100;
            session.setAttribute(VELOCIDAD_SESSION_KEY, v);
            mensajeWS = "{\"tipo\":\"velocidad\",\"valor\":\"" + v + "\"}";
        }
        // ===== VELOCIDAD MENOS (resta 25) =====
        else if ("velocidad_menos".equals(comando)) {
            Integer vActual = (Integer) session.getAttribute(VELOCIDAD_SESSION_KEY);
            if (vActual == null) vActual = 0;
            int vNuevo = vActual - 25;
            if (vNuevo < -100) vNuevo = -100;
            session.setAttribute(VELOCIDAD_SESSION_KEY, vNuevo);
            mensajeWS = "{\"tipo\":\"velocidad_menos\",\"valor\":\"\"}";
        }
        // ===== VELOCIDAD STOP =====
        else if ("velocidad_stop".equals(comando)) {
            mensajeWS = "{\"tipo\":\"velocidad_stop\",\"valor\":\"\"}";
        }
        // ===== DIRECCION (izquierda/derecha/stop) =====
        else if ("izquierda".equals(comando) || "derecha".equals(comando) || "stop".equals(comando)) {
            mensajeWS = "{\"tipo\":\"direccion\",\"valor\":\"" + comando + "\"}";
        }
        // ===== FRENO ROJO (rapido) =====
        else if ("freno".equals(comando)) {
            session.setAttribute(VELOCIDAD_SESSION_KEY, 0);
            mensajeWS = "{\"tipo\":\"freno\",\"valor\":\"\"}";
        }
        // ===== FRENO AMARILLO (lento) =====
        else if ("stop_secuencial".equals(comando)) {
            mensajeWS = "{\"tipo\":\"stop_secuencial\",\"valor\":\"\"}";
        }
        // ===== LUCES =====
        else if ("luces_altas".equals(comando) || "luces_bajas".equals(comando) ||
                 "luces_motor".equals(comando) || "luces_techo".equals(comando) ||
                 "luces_tablero".equals(comando) || "luces_bajas_atras".equals(comando)) {
            mensajeWS = "{\"tipo\":\"" + comando + "\",\"valor\":\"\"}";
        }
        // ===== SONIDO =====
        else if ("sonido".equals(comando)) {
            mensajeWS = "{\"tipo\":\"sonido\",\"valor\":\"\"}";
        }
        // ===== RESET =====
        else if ("reset".equals(comando)) {
            session.setAttribute(VELOCIDAD_SESSION_KEY, 0);
            session.setAttribute(MOTOR_SESSION_KEY, false);
            session.setAttribute(SISTEMA_SESSION_KEY, false);
            mensajeWS = "{\"tipo\":\"reset\",\"valor\":\"\"}";
        }
        // ===== POR DEFECTO =====
        else {
            mensajeWS = "{\"tipo\":\"direccion\",\"valor\":\"" + comando + "\"}";
        }

        ComandoWebSocketHandler.enviarComando(vehiculoId, mensajeWS);

        // Guardar en BD
        Comando cmd = new Comando();
        cmd.setVehiculoId(vehiculoId);
        cmd.setUsuarioId(usuarioId);
        cmd.setFecha(LocalDateTime.now());
        cmd.setTipo(comando);
        cmd.setValor(valor != null ? valor : "");

        comandoRepository.save(cmd);
        System.out.println("Comando guardado - Usuario: " + usuarioId + ", Vehículo: " + vehiculoId + ", Comando: " + cmd.getTipo() + " = " + cmd.getValor());

        return "redirect:/web/control?t=" + System.currentTimeMillis();
    }
}