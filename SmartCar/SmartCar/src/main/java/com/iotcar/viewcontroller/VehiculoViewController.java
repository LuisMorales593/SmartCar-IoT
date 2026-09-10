package com.iotcar.viewcontroller;

import com.iotcar.entity.Vehiculo;
import com.iotcar.repository.VehiculoRepository;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.ui.Model;
import org.springframework.web.bind.annotation.*;

import java.time.LocalDateTime;

@Controller
public class VehiculoViewController {

	@Autowired
	private VehiculoRepository repository;

	@GetMapping("/web/vehiculos")
	public String listar(Model model) {
		model.addAttribute("vehiculos", repository.findAll());
		model.addAttribute("vehiculo", new Vehiculo());
		model.addAttribute("contenido", "vehiculos");
		return "layout";
	}

	@PostMapping("/web/vehiculos/guardar")
	public String guardar(@ModelAttribute Vehiculo vehiculo) {
		if (vehiculo.getUltimaConexion() == null) {
			vehiculo.setUltimaConexion(LocalDateTime.now());
		}
		repository.save(vehiculo);
		return "redirect:/web/vehiculos";
	}

	@GetMapping("/web/vehiculos/editar/{id}")
	public String editar(@PathVariable Long id, Model model) {
		Vehiculo vehiculo = repository.findById(id).orElse(null);
		if (vehiculo == null) {
			return "redirect:/web/vehiculos";
		}
		model.addAttribute("vehiculo", vehiculo);
		model.addAttribute("vehiculos", repository.findAll());
		model.addAttribute("contenido", "vehiculos");
		return "layout";
	}

	@GetMapping("/web/vehiculos/eliminar/{id}")
	public String eliminar(@PathVariable Long id) {
		repository.deleteById(id);
		return "redirect:/web/vehiculos";
	}
}