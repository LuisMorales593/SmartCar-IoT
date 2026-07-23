package com.iotcar.viewcontroller;

import com.iotcar.entity.Sesion;
import com.iotcar.entity.Usuario;
import com.iotcar.repository.SesionRepository;
import com.iotcar.repository.UsuarioRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestParam;

import java.time.LocalDateTime;

@Controller
public class LoginViewController {

    @Autowired
    private UsuarioRepository usuarioRepository;

    @Autowired
    private SesionRepository sesionRepository;

    @GetMapping("/login")
    public String login() {
        return "login";
    }

    @PostMapping("/login")
    public String procesarLogin(@RequestParam String email,
                                @RequestParam String password,
                                Model model) {
        // Buscar usuario por email
        Usuario usuario = usuarioRepository.findByEmail(email).orElse(null);
        if (usuario == null || !usuario.getPassword().equals(password)) {
            model.addAttribute("error", "Credenciales inválidas");
            return "login";
        }

        // Crear sesión (sin vehículo por ahora, se asigna después)
        Sesion sesion = new Sesion();
        sesion.setUsuario(usuario);
        sesion.setFechaInicio(LocalDateTime.now());
        sesion.setActiva(true);
        sesionRepository.save(sesion);

        // Redirigir al panel de vehículos (o dashboard)
        return "redirect:/web/vehiculos";
    }
}