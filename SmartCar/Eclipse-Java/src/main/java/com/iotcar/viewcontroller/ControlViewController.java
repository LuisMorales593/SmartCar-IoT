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

        // 🔹 Obtener usuarioId de la sesión (en lugar de usar 1L fijo)
        Long usuarioId = (Long) session.getAttribute("usuarioId");
        if (usuarioId == null) {
            usuarioId = 1L; // fallback (por si no hay sesión)
        }

        // 1. Enviar por WebSocket
        String mensajeWS;
        if ("velocidad".equals(comando)) {
            mensajeWS = "{\"tipo\":\"velocidad\",\"valor\":\"" + (valor != null ? valor : "50") + "\"}";
        } else {
            mensajeWS = "{\"tipo\":\"direccion\",\"valor\":\"" + comando + "\"}";
        }
        ComandoWebSocketHandler.enviarComando(vehiculoId, mensajeWS);

        // 2. Guardar en base de datos
        Comando cmd = new Comando();
        cmd.setVehiculoId(vehiculoId);
        cmd.setUsuarioId(usuarioId);  // ← ahora usa el ID real del usuario
        cmd.setFecha(LocalDateTime.now());

        if ("velocidad".equals(comando)) {
            cmd.setTipo("velocidad");
            cmd.setValor(valor != null ? valor : "50");
        } else {
            cmd.setTipo("direccion");
            cmd.setValor(comando);
        }

        comandoRepository.save(cmd);
        System.out.println("Comando guardado - Usuario: " + usuarioId + ", Vehículo: " + vehiculoId + ", Comando: " + cmd.getTipo() + " = " + cmd.getValor());

        return "redirect:/web/control";
    }
}