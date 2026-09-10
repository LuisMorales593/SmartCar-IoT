package com.iotcar.viewcontroller;

import com.iotcar.entity.Usuario;
import com.iotcar.repository.UsuarioRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.*;

@Controller
public class UsuarioViewController {

    @Autowired
    private UsuarioRepository repository;

    @GetMapping("/web/usuarios")
    public String listar(Model model) {
        model.addAttribute("usuarios", repository.findAll());
        model.addAttribute("usuario", new Usuario());
        model.addAttribute("contenido", "usuarios");
        return "layout";
    }

    @PostMapping("/web/usuarios/guardar")
    public String guardar(@ModelAttribute Usuario usuario) {
        repository.save(usuario);
        return "redirect:/web/usuarios";
    }

    @GetMapping("/web/usuarios/editar/{id}")
    public String editar(@PathVariable Long id, Model model) {
        Usuario usuario = repository.findById(id).orElse(null);
        if (usuario == null) {
            return "redirect:/web/usuarios";
        }
        model.addAttribute("usuario", usuario);
        model.addAttribute("usuarios", repository.findAll());
        model.addAttribute("contenido", "usuarios");
        return "layout";
    }

    @GetMapping("/web/usuarios/eliminar/{id}")
    public String eliminar(@PathVariable Long id) {
        repository.deleteById(id);
        return "redirect:/web/usuarios";
    }
}