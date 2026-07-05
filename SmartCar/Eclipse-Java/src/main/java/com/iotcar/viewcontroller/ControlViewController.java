package com.iotcar.viewcontroller;

import com.iotcar.entity.Comando;
import com.iotcar.entity.Sesion;
import com.iotcar.entity.Vehiculo;
import com.iotcar.repository.ComandoRepository;
import com.iotcar.repository.SesionRepository;
import com.iotcar.repository.VehiculoRepository;
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
    public String control(@RequestParam(required = false) Long vehiculoId, Model model) {
        model.addAttribute("contenido", "control");
        model.addAttribute("vehiculoId", vehiculoId);

        if (vehiculoId != null) {
            Vehiculo vehiculo = vehiculoRepository.findById(vehiculoId).orElse(null);
            if (vehiculo != null) {
                model.addAttribute("vehiculoConectado", "CONECTADO".equals(vehiculo.getEstado()));
                // Buscar sesión activa para este vehículo
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
                                 @RequestParam(required = false) String valor) {
        Comando cmd = new Comando();
        cmd.setVehiculoId(vehiculoId);
        cmd.setUsuarioId(1L);
        cmd.setFecha(LocalDateTime.now());

        if ("velocidad".equals(comando)) {
            cmd.setTipo("velocidad");
            cmd.setValor(valor != null ? valor : "50");
        } else {
            cmd.setTipo("direccion");
            cmd.setValor(comando);
        }

        comandoRepository.save(cmd);
        System.out.println("Comando guardado: " + cmd.getTipo() + " = " + cmd.getValor());

        return "redirect:/web/control?vehiculoId=" + vehiculoId;
    }
}