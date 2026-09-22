package com.hkt.devicehub.domain.repository;

import com.hkt.devicehub.domain.model.DeviceProject;
import com.hkt.devicehub.domain.model.DisposalTicket;
import org.springframework.data.jpa.repository.JpaRepository;

import java.util.Collection;
import java.util.List;
import java.util.Optional;

public interface DisposalTicketRepository extends JpaRepository<DisposalTicket, Long> {

    Optional<DisposalTicket> findFirstByDevEuiAndStatusIn(String devEui,
                                                          Collection<DisposalTicket.Status> statuses);

    List<DisposalTicket> findByStatus(DisposalTicket.Status status);

    List<DisposalTicket> findByStatusAndProject(DisposalTicket.Status status, DeviceProject project);

    List<DisposalTicket> findByProject(DeviceProject project);
}
