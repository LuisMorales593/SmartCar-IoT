package com.iotcar.repository;

import com.iotcar.entity.Sesion;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.stereotype.Repository;

import java.util.List;
import java.util.Optional;

@Repository
public interface SesionRepository extends JpaRepository<Sesion, Long> {
    Optional<Sesion> findFirstByUsuarioIdAndActivaTrueOrderByFechaInicioDesc(Long usuarioId);
    List<Sesion> findByUsuarioIdAndActivaTrue(Long usuarioId);
    List<Sesion> findByVehiculoIdAndActivaTrue(Long vehiculoId);   // ← NUEVO
}