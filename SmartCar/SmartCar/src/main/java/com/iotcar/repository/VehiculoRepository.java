package com.iotcar.repository;

import org.springframework.data.jpa.repository.JpaRepository;

import com.iotcar.entity.Vehiculo;

public interface VehiculoRepository extends JpaRepository<Vehiculo, Long> {

}