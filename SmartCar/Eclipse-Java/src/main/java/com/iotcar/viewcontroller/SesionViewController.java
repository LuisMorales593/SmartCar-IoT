package com.iotcar.viewcontroller;

import com.iotcar.entity.Sesion;
import com.iotcar.entity.Usuario;
import com.iotcar.entity.Vehiculo;
import com.iotcar.repository.SesionRepository;
import com.iotcar.repository.UsuarioRepository;
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
public class SesionViewController {

    @Autowired
    private UsuarioRepository usuarioRepository;

    @Autowired
    private VehiculoRepository vehiculoRepository;

    @Autowired
    private SesionRepository sesionRepository;

    @GetMapping("/web/sesion")
    public String mostrarFormulario(Model model) {
        List<Usuario> usuarios = usuarioRepository.findAll();
        List<Vehiculo> vehiculos = vehiculoRepository.findAll();
        model.addAttribute("usuarios", usuarios);
        model.addAttribute("vehiculos", vehiculos);
        model.addAttribute("contenido", "sesion");
        return "layout";
    }

    @PostMapping("/web/sesion/iniciar")
    public String iniciarSesion(@RequestParam Long usuarioId,
                                @RequestParam Long vehiculoId) {
        // Cerrar sesiones activas anteriores del mismo usuario
        List<Sesion> sesionesActivas = sesionRepository.findByUsuarioIdAndActivaTrue(usuarioId);
        for (Sesion s : sesionesActivas) {
            s.setActiva(false);
            s.setFechaFin(LocalDateTime.now());
            sesionRepository.save(s);
        }

        Usuario usuario = usuarioRepository.findById(usuarioId).orElse(null);
        Vehiculo vehiculo = vehiculoRepository.findById(vehiculoId).orElse(null);

        if (usuario == null || vehiculo == null) {
            return "redirect:/web/sesion?error=1";
        }

        Sesion sesion = new Sesion();
        sesion.setUsuario(usuario);
        sesion.setVehiculo(vehiculo);
        sesion.setFechaInicio(LocalDateTime.now());
        sesion.setActiva(true);
        sesionRepository.save(sesion);

        return "redirect:/web/control?vehiculoId=" + vehiculoId;
    }

    @GetMapping("/web/sesion/cerrar")
    public String cerrarSesion(@RequestParam Long sesionId) {
        Sesion sesion = sesionRepository.findById(sesionId).orElse(null);
        if (sesion != null && sesion.getActiva()) {
            sesion.setActiva(false);
            sesion.setFechaFin(LocalDateTime.now());
            sesionRepository.save(sesion);
        }
        return "redirect:/web/sesion";
    }
}