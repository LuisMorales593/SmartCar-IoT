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

        if ("arrancar".equals(comando)) {
            boolean encendido = "on".equals(valor);
            session.setAttribute(MOTOR_SESSION_KEY, encendido);
            mensajeWS = "{\"tipo\":\"arrancar\",\"valor\":\"" + (encendido ? "on" : "off") + "\"}";
        } else if ("velocidad".equals(comando)) {
            int v = Integer.parseInt(valor);
            // Permitir valores negativos para retroceso
            if (v < -100) v = -100;
            if (v > 100) v = 100;
            session.setAttribute(VELOCIDAD_SESSION_KEY, v);
            mensajeWS = "{\"tipo\":\"velocidad\",\"valor\":\"" + v + "\"}";
        } else {
            mensajeWS = "{\"tipo\":\"direccion\",\"valor\":\"" + comando + "\"}";
        }

        ComandoWebSocketHandler.enviarComando(vehiculoId, mensajeWS);

        // Guardar en BD
        Comando cmd = new Comando();
        cmd.setVehiculoId(vehiculoId);
        cmd.setUsuarioId(usuarioId);
        cmd.setFecha(LocalDateTime.now());

        if ("arrancar".equals(comando) || "velocidad".equals(comando) || "freno".equals(comando) || "luces".equals(comando) || "bocina".equals(comando) || "animacion".equals(comando)) {
            cmd.setTipo(comando);
            cmd.setValor(valor != null ? valor : "");
        } else {
            cmd.setTipo("direccion");
            cmd.setValor(comando);
        }

        comandoRepository.save(cmd);
        System.out.println("Comando guardado - Usuario: " + usuarioId + ", Vehículo: " + vehiculoId + ", Comando: " + cmd.getTipo() + " = " + cmd.getValor());

        // 🔥 FORZAR RECARGA PARA QUE SE VEA EL VALOR NEGATIVO 🔥
        return "redirect:/web/control?t=" + System.currentTimeMillis();
    }
}