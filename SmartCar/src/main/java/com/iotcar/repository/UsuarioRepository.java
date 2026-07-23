package com.iotcar.repository;

import com.iotcar.entity.Usuario;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.Optional;

//Indica que esta interfaz se encargará del acceso a la base de datos.
//Al extender JpaRepository Spring Boot crea automáticamente
//los métodos básicos para trabajar con la tabla Usuarios:
//
//save() -> Guardar un usuario.
//findAll() -> Obtener todos los usuarios.
//findById() -> Buscar un usuario por su id.
//deleteById()-> Eliminar un usuario por su id.
//count() -> Contar cuántos usuarios existen.

@Repository
public interface UsuarioRepository extends JpaRepository<Usuario, Long> {

    Optional<Usuario> findByEmail(String email);
}