package com.iotcar.viewcontroller;

import com.iotcar.repository.ComandoRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.GetMapping;

@Controller
public class ComandoViewController {

    @Autowired
    private ComandoRepository repository;

    @GetMapping("/web/comandos")
    public String listar(Model model) {
        model.addAttribute("comandos", repository.findAll());
        model.addAttribute("contenido", "comandos");
        return "layout";
    }
}