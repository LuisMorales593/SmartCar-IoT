package com.iotcar.repository;

import java.util.List;

import org.springframework.data.jpa.repository.JpaRepository;
import com.iotcar.entity.Comando;

public interface ComandoRepository extends JpaRepository<Comando, Long> {
	List<Comando> findByVehiculoId(Long vehiculoId);

}