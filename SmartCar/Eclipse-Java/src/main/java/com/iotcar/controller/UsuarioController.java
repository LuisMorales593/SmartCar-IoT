package com.iotcar.controller;

import com.iotcar.entity.Usuario;
import com.iotcar.repository.UsuarioRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.web.bind.annotation.*;

import java.util.List;

// Recibe las peticiones HTTP del cliente y las procesa.De momento solo haremos el GET
//para comprobar que todo está conectado correctamente. 
//Cuando funcione, añadiremos POST, PUT y DELETE
@RestController
@RequestMapping("/usuarios")
public class UsuarioController {

    @Autowired
    private UsuarioRepository usuarioRepository;

    // Devuelve la lista de todos los usuarios.
    @GetMapping
    public List<Usuario> obtenerUsuarios() {
        return usuarioRepository.findAll();
    }
}