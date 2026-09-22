package com.hkt.devicehub.domain.repository;

import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import org.springframework.data.domain.Page;
import org.springframework.data.domain.Pageable;
import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;

import java.util.List;
import java.util.Optional;

public interface RegisteredDeviceRepository extends JpaRepository<RegisteredDevice, Long> {

    Optional<RegisteredDevice> findByProjectAndDevEui(DeviceProject project, String devEui);

    List<RegisteredDevice> findByDevEui(String devEui);

    List<RegisteredDevice> findByProject(DeviceProject project);

    List<RegisteredDevice> findByStatus(RegistrationStatus status);

    long countByStatus(RegistrationStatus status);

    @Query("SELECT d FROM RegisteredDevice d "
            + "WHERE (:project IS NULL OR d.project = :project) "
            + "AND (:status IS NULL OR d.status = :status) "
            + "AND (:q IS NULL OR lower(d.devEui) LIKE %:q% "
            + "OR lower(d.externalRef) LIKE %:q% OR lower(d.deviceType) LIKE %:q%)")
    Page<RegisteredDevice> search(@Param("project") DeviceProject project,
                                  @Param("status") RegistrationStatus status,
                                  @Param("q") String q,
                                  Pageable pageable);
}
