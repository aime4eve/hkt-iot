package com.hkt.devicehub.application;

import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.config.DeviceHubProperties;
import com.hkt.devicehub.infrastructure.ns.NsClient;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import lombok.extern.slf4j.Slf4j;
import org.springframework.stereotype.Service;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;

/**
 * Five-check onboarding preflight (R-01, console §3.3). Each check returns
 * PASS/WARN/FAIL plus a Chinese fix suggestion and the evidence fields it
 * relied on. Verdict semantics:
 * ① NS 存在性与归属 — 不存在=FAIL；
 * ② 网关 OC 映射 — 该 NS 项目无映射=FAIL（上行到不了 TB，L4）；
 * ③ TB 设备唯一性 — 恰好 1 台=PASS，0 台=WARN 可创建，多台=FAIL 列候选（L1/L2）；
 * ④ profile 匹配 — 设备已存在但 profile 与类型映射不符=FAIL（L3）；
 * ⑤ DeviceHub 绑定状态 — ACTIVE=PASS，未注册=WARN。
 */
@Service
@Slf4j
public class PreflightService {

    private final NsClient nsClient;
    private final TbClient tbClient;
    private final GatewayMappingService gatewayMapping;
    private final RegisteredDeviceRepository deviceRepository;
    private final DeviceHubProperties properties;

    public PreflightService(NsClient nsClient, TbClient tbClient,
                            GatewayMappingService gatewayMapping,
                            RegisteredDeviceRepository deviceRepository,
                            DeviceHubProperties properties) {
        this.nsClient = nsClient;
        this.tbClient = tbClient;
        this.gatewayMapping = gatewayMapping;
        this.deviceRepository = deviceRepository;
        this.properties = properties;
    }

    public enum Verdict { PASS, WARN, FAIL }

    public record CheckResult(String key, Verdict verdict, String advice,
                              Map<String, Object> evidence) {}

    public record PreflightReport(String devEui, List<CheckResult> checks, Verdict overall) {}

    public PreflightReport preflight(String devEui, String deviceType) {
        String eui = DeviceProvisioningServiceRef.normalize(devEui);
        if (!eui.matches("^[0-9a-f]{16}$")) {
            throw new IllegalArgumentException("invalid devEui format: " + devEui);
        }

        CheckResult nsCheck = checkNs(eui);
        CheckResult mappingCheck = checkGatewayMapping(nsCheck);
        CheckResult uniquenessCheck = checkTbUniqueness(eui);
        CheckResult profileCheck = checkProfile(eui, deviceType, uniquenessCheck);
        CheckResult bindingCheck = checkLocalBinding(eui);

        List<CheckResult> checks = List.of(
                nsCheck, mappingCheck, uniquenessCheck, profileCheck, bindingCheck);
        Verdict overall = checks.stream().map(CheckResult::verdict)
                .max(Enum::compareTo).orElse(Verdict.PASS);
        // Enum order PASS < WARN < FAIL — worst verdict wins.
        return new PreflightReport(eui, checks, overall);
    }

    CheckResult checkNs(String eui) {
        if (!nsClient.isEnabled()) {
            return new CheckResult("NS_DEVICE", Verdict.WARN,
                    "NS 检查未启用（devicehub.ns.enabled=false），无法校验归属", Map.of());
        }
        try {
            Optional<NsClient.NsDevice> device = nsClient.findDeviceByEui(eui);
            if (device.isEmpty()) {
                return new CheckResult("NS_DEVICE", Verdict.FAIL,
                        "NS 中不存在该设备，需先在 NS 完成入网", Map.of());
            }
            NsClient.NsDevice ns = device.get();
            Map<String, Object> evidence = new LinkedHashMap<>();
            evidence.put("nsProjectId", ns.projectId());
            evidence.put("nsAppId", ns.appId());
            evidence.put("nsName", ns.name());
            evidence.put("nsOnline", ns.online());
            // L6: f_cnt_up may disagree with the NS UI — evidence only.
            evidence.put("nsFrameCountUp", ns.fCntUp());
            return new CheckResult("NS_DEVICE", Verdict.PASS,
                    "设备已入网，归属 NS 项目 " + ns.projectId(), evidence);
        } catch (Exception e) {
            return new CheckResult("NS_DEVICE", Verdict.WARN,
                    "NS 查询失败：" + e.getMessage(), Map.of());
        }
    }

    CheckResult checkGatewayMapping(CheckResult nsCheck) {
        Object projectId = nsCheck.evidence().get("nsProjectId");
        if (!(projectId instanceof Integer nsProjectId)) {
            return new CheckResult("GATEWAY_MAPPING", Verdict.WARN,
                    "NS 侧未确认设备归属，跳过网关映射检查", Map.of());
        }
        try {
            if (gatewayMapping.isMapped(nsProjectId)) {
                return new CheckResult("GATEWAY_MAPPING", Verdict.PASS,
                        "NS 项目 " + nsProjectId + " 已有网关 OC topic 映射",
                        Map.of("nsProjectId", nsProjectId));
            }
            return new CheckResult("GATEWAY_MAPPING", Verdict.FAIL,
                    "项目 " + nsProjectId + " 无网关映射，上行不会到达 TB；"
                            + "需走映射变更工单（备份→变更→重载→验证）",
                    Map.of("nsProjectId", nsProjectId));
        } catch (Exception e) {
            return new CheckResult("GATEWAY_MAPPING", Verdict.WARN,
                    "网关共享属性读取失败：" + e.getMessage(), Map.of("nsProjectId", nsProjectId));
        }
    }

    CheckResult checkTbUniqueness(String eui) {
        List<TbClient.TbDeviceView> matches = tbClient.findDevices(eui);
        if (matches.isEmpty()) {
            return new CheckResult("TB_UNIQUENESS", Verdict.WARN,
                    "TB 无同名设备（含大小写变体），注册时将新建", Map.of("candidates", List.of()));
        }
        if (matches.size() == 1) {
            TbClient.TbDeviceView only = matches.get(0);
            return new CheckResult("TB_UNIQUENESS", Verdict.PASS,
                    "TB 已有唯一同名设备，注册将绑定现有设备",
                    Map.of("tbDeviceId", only.id(), "tbName", only.name()));
        }
        return new CheckResult("TB_UNIQUENESS", Verdict.FAIL,
                "TB 存在 " + matches.size() + " 台同名/大小写变体设备，需先在治理中心清理空副本",
                Map.of("candidates", matches.stream().map(TbClient.TbDeviceView::id).toList()));
    }

    CheckResult checkProfile(String eui, String deviceType, CheckResult uniquenessCheck) {
        String type = deviceType;
        if (type == null || type.isBlank()) {
            type = deviceRepository.findByDevEui(eui).stream()
                    .map(RegisteredDevice::getDeviceType)
                    .filter(t -> t != null && !t.isBlank())
                    .findFirst().orElse(null);
        }
        if (type == null) {
            return new CheckResult("PROFILE_MATCH", Verdict.WARN,
                    "未提供 deviceType，无法核对 profile（注册时建议携带，缺失将挂默认 profile）",
                    Map.of());
        }
        DeviceHubProperties.ProfileRef expected =
                properties.getProfiles().get(type.toUpperCase(java.util.Locale.ROOT));
        if (expected == null || expected.getName() == null) {
            return new CheckResult("PROFILE_MATCH", Verdict.WARN,
                    "未配置 deviceType " + type + " 的 profile 映射（devicehub.profiles）",
                    Map.of("deviceType", type));
        }
        try {
            Map<String, String> profiles = tbClient.fetchDeviceProfiles();
            String expectedId = profiles.entrySet().stream()
                    .filter(entry -> entry.getValue().equals(expected.getName()))
                    .map(Map.Entry::getKey).findFirst().orElse(null);
            if (expectedId == null) {
                return new CheckResult("PROFILE_MATCH", Verdict.FAIL,
                        "TB 中不存在 profile「" + expected.getName() + "」，解码链无法工作（L3）",
                        Map.of("deviceType", type, "expectedProfile", expected.getName()));
            }
            Map<String, Object> evidence = new LinkedHashMap<>();
            evidence.put("deviceType", type);
            evidence.put("expectedProfile", expected.getName());
            Object tbDeviceId = uniquenessCheck.evidence().get("tbDeviceId");
            if (tbDeviceId instanceof String id) {
                String actualProfileId = tbClient.findDevices(eui).stream()
                        .filter(view -> view.id().equals(id))
                        .map(TbClient.TbDeviceView::profileId)
                        .findFirst().orElse(null);
                evidence.put("actualProfileId", actualProfileId);
                if (actualProfileId != null && !actualProfileId.equals(expectedId)) {
                    return new CheckResult("PROFILE_MATCH", Verdict.FAIL,
                            "TB 设备当前 profile 与类型不匹配，期望「" + expected.getName() + "」",
                            evidence);
                }
            }
            return new CheckResult("PROFILE_MATCH", Verdict.PASS,
                    "profile 映射正确：「" + expected.getName() + "」", evidence);
        } catch (Exception e) {
            return new CheckResult("PROFILE_MATCH", Verdict.WARN,
                    "profile 核对失败：" + e.getMessage(), Map.of("deviceType", type));
        }
    }

    CheckResult checkLocalBinding(String eui) {
        List<RegisteredDevice> locals = deviceRepository.findByDevEui(eui);
        if (locals.isEmpty()) {
            return new CheckResult("LOCAL_BINDING", Verdict.WARN,
                    "DeviceHub 未注册该设备，可执行一键注册", Map.of());
        }
        RegisteredDevice active = locals.stream()
                .filter(d -> d.getStatus() == RegistrationStatus.ACTIVE)
                .findFirst().orElse(null);
        if (active != null) {
            Map<String, Object> evidence = new LinkedHashMap<>();
            evidence.put("project", active.getProject().name());
            evidence.put("tbDeviceId", active.getTbDeviceId() == null
                    ? null : active.getTbDeviceId().toString());
            evidence.put("telemetryCursorMs", active.getTelemetryCursorMs());
            return new CheckResult("LOCAL_BINDING", Verdict.PASS,
                    "已注册并激活（" + active.getProject() + "）", evidence);
        }
        return new CheckResult("LOCAL_BINDING", Verdict.WARN,
                "本地存在非激活记录（" + locals.get(0).getStatus() + "），重新注册可修复",
                Map.of("status", locals.get(0).getStatus().name()));
    }

    /** Small indirection so tests need no static call into provisioning. */
    static final class DeviceProvisioningServiceRef {
        private DeviceProvisioningServiceRef() {}
        static String normalize(String eui) {
            return eui == null ? "" : eui.trim().toLowerCase(java.util.Locale.ROOT);
        }
    }
}
