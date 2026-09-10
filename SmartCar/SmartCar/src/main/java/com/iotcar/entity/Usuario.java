package com.iotcar.entity;

import jakarta.persistence.*;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;

//Representa la tabla Usuarios de la base de datos como un objeto Java.
@Entity
@Table(name = "usuarios")
@JsonIgnoreProperties({"hibernateLazyInitializer", "handler"})
public class Usuario {

	@Id
	@GeneratedValue(strategy = GenerationType.IDENTITY)
	private Long id;

	private String nombre;

	private String email;

	private String password;

	private String rol;
	private Boolean activo;

	public Usuario() {
	}

	public Usuario(Long id, String nombre, String email, String password, String rol, Boolean activo) {
	    this.id = id;
	    this.nombre = nombre;
	    this.email = email;
	    this.password = password;
	    this.rol = rol;
	    this.activo = activo;
	}

	public Long getId() {
		return id;
	}

	public void setId(Long id) {
		this.id = id;
	}

	public String getNombre() {
		return nombre;
	}

	public void setNombre(String nombre) {
		this.nombre = nombre;
	}

	public String getEmail() {
		return email;
	}

	public void setEmail(String email) {
		this.email = email;
	}

	public void setPassword(String password) {
		this.password = password;
	}

	public String getPassword() {
		return password;
	}

	public String getRol() {
		return rol;
	}

	public void setRol(String rol) {
		this.rol = rol;
	}
	public Boolean getActivo() {
	    return activo;
	}

	public void setActivo(Boolean activo) {
	    this.activo = activo;
	}
}
