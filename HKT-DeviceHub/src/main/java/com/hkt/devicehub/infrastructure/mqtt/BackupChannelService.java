package com.hkt.devicehub.infrastructure.mqtt;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.hkt.devicehub.domain.model.RegistrationStatus;
import com.hkt.devicehub.domain.model.RegisteredDevice;
import com.hkt.devicehub.domain.repository.RegisteredDeviceRepository;
import com.hkt.devicehub.infrastructure.thingsboard.TbClient;
import com.hkt.devicehub.infrastructure.thingsboard.TbProperties;
import jakarta.annotation.PostConstruct;
import jakarta.annotation.PreDestroy;
import lombok.extern.slf4j.Slf4j;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;
import org.springframework.stereotype.Component;

import java.nio.charset.StandardCharsets;
import java.time.Instant;
import java.time.LocalDate;
import java.time.ZoneId;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.Executors;
import java.util.concurrent.RejectedExecutionException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;

/**
 * NS→TB backup telemetry channel (docs/ns-direct-tb-backup-channel.md):
 * wildcard-subscribes the shared NS MQTT bus and writes every frame of a
 * whitelisted, registry-matched device straight into TB as a {@code dataHex}
 * point with explicit ts. During normal operation the 2s same-frame window in
 * {@code TbFrameNormalizer} merges these points with the bridge's result
 * frames; when the bridge drops frames, the point becomes a fallback frame —
 * business data survives with business fields degraded.
 * <p>
 * Design invariants:
 * <ul>
 *   <li>The channel must never become a new silent-dropping point: every
 *       discard is counted (parseFailed/otherIgnored/notWhitelisted/
 *       unknownDevice/inactiveDevice/ambiguousMatch/writeFailed/
 *       queueOverflow) and visible on {@code /api/v1/backup-channel/status}.</li>
 *   <li>Independent clientId — same-id reconnects would kick the live bridge.</li>
 *   <li>cleanSession=false + QoS1: while we are disconnected the broker
 *       queues our messages and replays them on reconnect (verified in the
 *       Phase 0 spike), so reconnects delay frames but never lose them.</li>
 *   <li>Reconnect strategy is a full client rebuild by the watchdog
 *       (single reconnect path), not paho automaticReconnect.</li>
 *   <li>Unknown devices are counted, never written (first all-bus traffic
 *       view DeviceHub has).</li>
 * </ul>
 */
@Component
@Slf4j
public class BackupChannelService implements MqttCallback {

    private static final long CONNECT_TIMEOUT_SECONDS = 30;
    private static final long WATCHDOG_DELAY_SECONDS = 30;

    public enum State { STOPPED, CONNECTING, CONNECTED, DISCONNECTED, FAILED }

    private final BackupChannelProperties properties;
    private final TbClient tbClient;
    private final TbProperties tbProperties;
    private final ObjectMapper objectMapper;
    private final RegisteredDeviceRepository deviceRepository;

    private final Stats stats = new Stats();
    private final Map<String, CacheEntry> matchCache = new ConcurrentHashMap<>();

    private volatile MqttClient client;
    /** Runtime switch, seeded from config at boot; the console toggle moves this. */
    private volatile boolean runtimeEnabled;
    /** Connect gate: only one attemptConnect runs at a time (start() and the watchdog both queue it). */
    private final java.util.concurrent.atomic.AtomicBoolean connecting =
            new java.util.concurrent.atomic.AtomicBoolean(false);
    private volatile State state = State.STOPPED;
    private volatile boolean subscribed;
    private volatile String brokerHost;
    private volatile int brokerPort;
    private volatile Set<Long> whitelist = Set.of();
    private volatile ThreadPoolExecutor writer;

    private final ScheduledExecutorService watchdog =
            Executors.newSingleThreadScheduledExecutor(r -> {
                Thread t = new Thread(r, "backup-channel-watchdog");
                t.setDaemon(true);
                return t;
            });

    public BackupChannelService(BackupChannelProperties properties, TbClient tbClient,
                                TbProperties tbProperties, ObjectMapper objectMapper,
                                RegisteredDeviceRepository deviceRepository) {
        this.properties = properties;
        this.tbClient = tbClient;
        this.tbProperties = tbProperties;
        this.objectMapper = objectMapper;
        this.deviceRepository = deviceRepository;
        // eager: idle daemon pool, only used once frames flow; capacity from config
        this.writer = new ThreadPoolExecutor(1, 1, 60, TimeUnit.SECONDS,
                new ArrayBlockingQueue<>(Math.max(1, properties.getMaxQueue())),
                r -> {
                    Thread t = new Thread(r, "backup-channel-writer");
                    t.setDaemon(true);
                    return t;
                });
        this.writer.allowCoreThreadTimeOut(true);
        // effective from construction; start() re-reads config on (re)enable
        this.whitelist = properties.whitelistSet();
    }

    @PostConstruct
    void init() {
        watchdog.scheduleWithFixedDelay(this::watchdogTick,
                WATCHDOG_DELAY_SECONDS, WATCHDOG_DELAY_SECONDS, TimeUnit.SECONDS);
        if (properties.isEnabled()) {
            start();
        }
    }

    @PreDestroy
    void shutdown() {
        runtimeEnabled = false;
        stop();
        watchdog.shutdownNow();
    }

    /** Idempotent enable: brings the MQTT connection up (async, watchdog retries). */
    public synchronized void start() {
        runtimeEnabled = true;
        if (state == State.CONNECTED || state == State.CONNECTING) return;
        whitelist = properties.whitelistSet();
        state = State.CONNECTING;
        stats.startedAt = Instant.now();
        watchdog.execute(this::attemptConnect);
    }

    /** Idempotent disable: disconnects; the broker keeps queuing until re-enable. */
    public synchronized void stop() {
        runtimeEnabled = false;
        state = State.STOPPED;
        subscribed = false;
        closeQuietly();
    }

    public void switchEnabled(boolean enabled) {
        if (enabled) start(); else stop();
    }

    private void watchdogTick() {
        try {
            if (!runtimeEnabled || state == State.CONNECTING) return;
            MqttClient current = client;
            if (current != null && current.isConnected() && subscribed) return;
            attemptConnect();
        } catch (Exception e) {
            log.warn("[backup-channel] watchdog tick failed: {}", e.getMessage());
        }
    }

    private synchronized void attemptConnect() {
        // state==CONNECTING is NOT a reason to bail: start() sets it before
        // queueing this task — the CAS is the single re-entry guard.
        if (!runtimeEnabled || !connecting.compareAndSet(false, true)) return;
        try {
            doConnect();
        } finally {
            connecting.set(false);
        }
    }

    private void doConnect() {
        closeQuietly();
        state = State.CONNECTING;
        stats.connectionAttempts.incrementAndGet();
        MqttClient mqtt = null;
        try {
            String[] creds = resolveBroker();
            log.info("[backup-channel] connecting to {}:{} as clientId {}",
                    brokerHost, brokerPort, properties.getClientId());
            mqtt = new MqttClient(
                    "tcp://" + brokerHost + ":" + brokerPort,
                    properties.getClientId(), new MemoryPersistence());
            mqtt.setCallback(this);
            MqttConnectOptions options = new MqttConnectOptions();
            options.setUserName(creds[0]);
            options.setPassword(creds[1].toCharArray());
            options.setCleanSession(false);
            options.setAutomaticReconnect(false);
            options.setKeepAliveInterval(60);
            options.setConnectionTimeout((int) CONNECT_TIMEOUT_SECONDS);
            mqtt.connect(options);
            mqtt.subscribe(properties.getTopicFilter(), properties.getQos());
            if (!runtimeEnabled) {
                // stop() raced the connect — honour the switch-off
                try {
                    mqtt.disconnectForcibly(1_000);
                    mqtt.close();
                } catch (Exception ignored) {
                    // already gone
                }
                state = State.STOPPED;
                return;
            }
            client = mqtt;
            subscribed = true;
            state = State.CONNECTED;
            stats.lastConnectedAt = Instant.now();
            log.info("[backup-channel] connected and subscribed '{}' (QoS {}, whitelist {}, mode {})",
                    properties.getTopicFilter(), properties.getQos(), whitelist,
                    whitelist.isEmpty() ? "COUNT_ONLY" : "WRITE");
        } catch (Exception e) {
            state = State.FAILED;
            stats.lastError = e.getMessage();
            closeQuietly();
            log.warn("[backup-channel] connect failed (watchdog retries in {}s): {}",
                    WATCHDOG_DELAY_SECONDS, e.getMessage());
        }
    }

    /**
     * Broker coordinates/credentials come from the same OC shared attribute the
     * bridge uses (single source of truth, no secrets in config); the config
     * host/port are only a fallback for bootstrapping.
     */
    private String[] resolveBroker() {
        brokerHost = properties.getBrokerHost();
        brokerPort = properties.getBrokerPort();
        Map<String, JsonNode> attrs = tbClient.fetchSharedAttributes(tbProperties.getGatewayDeviceId());
        JsonNode oc = attrs.get(tbProperties.getMappingAttrKey());
        if (oc == null || oc.isNull()) {
            throw new IllegalStateException("gateway shared attribute '"
                    + tbProperties.getMappingAttrKey() + "' missing — cannot resolve broker");
        }
        JsonNode cfg = unwrap(oc);
        JsonNode cj = unwrap(cfg.path("configurationJson"));
        JsonNode broker = cj.path("broker");
        JsonNode security = broker.path("security");
        JsonNode hostNode = broker.path("host");
        if (hostNode.isTextual() && !hostNode.asText().isBlank()) brokerHost = hostNode.asText();
        JsonNode portNode = broker.path("port");
        if (portNode.isNumber()) brokerPort = portNode.asInt();
        String user = security.path("username").asText(null);
        String pass = security.path("password").asText(null);
        if (user == null || pass == null) {
            throw new IllegalStateException("OC broker.security incomplete — cannot resolve credentials");
        }
        return new String[]{user, pass};
    }

    /** TB shared-attribute values may be pre-parsed objects or JSON strings — both occur. */
    private JsonNode unwrap(JsonNode node) {
        if (node.isTextual()) {
            try {
                return objectMapper.readTree(node.asText());
            } catch (Exception e) {
                throw new IllegalStateException("OC attribute JSON string unparsable", e);
            }
        }
        return node;
    }

    private void closeQuietly() {
        MqttClient current = client;
        client = null;
        if (current == null) return;
        try {
            if (current.isConnected()) current.disconnectForcibly(1_000);
        } catch (Exception ignored) {
            // already gone
        }
        try {
            current.close();
        } catch (Exception ignored) {
            // already closed
        }
    }

    // ------------------------------------------------------------ MQTT callback

    @Override
    public void connectionLost(Throwable cause) {
        subscribed = false;
        if (state == State.STOPPED) return;
        state = State.DISCONNECTED;
        stats.connectionLosts.incrementAndGet();
        stats.lastError = cause == null ? null : cause.getMessage();
        log.warn("[backup-channel] connection lost (watchdog reconnects in ≤{}s, broker queues QoS1): {}",
                WATCHDOG_DELAY_SECONDS, cause == null ? "?" : cause.getMessage());
    }

    @Override
    public void messageArrived(String topic, MqttMessage message) {
        stats.received.incrementAndGet();
        try {
            handle(topic, message);
        } catch (Exception e) {
            // callback must never throw into the client thread
            stats.parseFailed.incrementAndGet();
            log.warn("[backup-channel] handle failed for {}: {}", topic, e.getMessage());
        }
    }

    @Override
    public void deliveryComplete(IMqttDeliveryToken token) {
        // we never publish
    }

    /** Topic contract: org/{org}/project/{id}/device/{eui}/dat/up; anything else ignored. */
    void handle(String topic, MqttMessage message) {
        String[] seg = topic.split("/");
        if (seg.length != 8 || !"project".equals(seg[2]) || !"device".equals(seg[4])
                || !"up".equals(seg[7])) {
            stats.otherIgnored.incrementAndGet();
            return;
        }
        long topicProject;
        try {
            topicProject = Long.parseLong(seg[3]);
        } catch (NumberFormatException e) {
            stats.otherIgnored.incrementAndGet();
            return;
        }
        NsUplinkMessage msg = NsUplinkMessage.parse(objectMapper, topicProject,
                new String(message.getPayload(), StandardCharsets.UTF_8));
        if (msg == null) {
            stats.parseFailed.incrementAndGet();
            return;
        }
        if (msg.payloadProject() != null && msg.payloadProject() != topicProject) {
            stats.projectMismatch.incrementAndGet();
        }
        long project = msg.payloadProject() != null ? msg.payloadProject() : topicProject;
        if (!whitelist.contains(project)) {
            stats.notWhitelisted.incrementAndGet();
            return;
        }
        CacheEntry cached = matchCache.get(msg.devEui());
        if (cached != null && cached.expiresAt > System.currentTimeMillis()) {
            applyMatch(cached.outcome(), cached.tbDeviceId(), msg);
            return;
        }
        MatchOutcome outcome = match(msg.devEui());
        matchCache.put(msg.devEui(), new CacheEntry(outcome.outcome(), outcome.tbDeviceId(),
                System.currentTimeMillis() + properties.getMatchCacheTtlMs()));
        applyMatch(outcome.outcome(), outcome.tbDeviceId(), msg);
    }

    private void applyMatch(MatchResult outcome, String tbDeviceId, NsUplinkMessage msg) {
        switch (outcome) {
            case UNKNOWN -> stats.unknownDevice.incrementAndGet();
            case INACTIVE -> stats.inactiveDevice.incrementAndGet();
            case AMBIGUOUS -> stats.ambiguousMatch.incrementAndGet();
            case MATCH -> enqueueWrite(tbDeviceId, msg);
        }
    }

    private void enqueueWrite(String tbDeviceId, NsUplinkMessage msg) {
        stats.matched.incrementAndGet();
        rollDay();
        stats.todayFrames.incrementAndGet();
        stats.lastFrameAt = Instant.now();
        String taskTbId = tbDeviceId;
        long tsMs = msg.tsMs();
        String dataHex = msg.dataHex();
        try {
            writer.execute(() -> {
                try {
                    tbClient.writeDeviceTelemetry(taskTbId, tsMs, dataHex);
                    stats.written.incrementAndGet();
                    stats.lastWriteAt = Instant.now();
                } catch (Exception e) {
                    stats.writeFailed.incrementAndGet();
                    stats.lastError = "write: " + e.getMessage();
                    log.warn("[backup-channel] TB write failed for {} at {}: {}",
                            taskTbId, tsMs, e.getMessage());
                }
            });
        } catch (RejectedExecutionException e) {
            // queue full or executor shut down — counted, never silent
            stats.queueOverflow.incrementAndGet();
        }
    }

    private record MatchOutcome(MatchResult outcome, String tbDeviceId) {}

    /**
     * Registry match: exactly one ACTIVE row with a TB binding wins. Multiple
     * ACTIVE rows are ambiguous data (e.g. the capsule FAILED+ACTIVE double-row
     * family) and are never silently auto-bound.
     */
    private MatchOutcome match(String devEui) {
        List<RegisteredDevice> rows = deviceRepository.findByDevEui(devEui);
        if (rows.isEmpty()) {
            if (log.isDebugEnabled()) log.debug("[backup-channel] unknown device {}", devEui);
            return new MatchOutcome(MatchResult.UNKNOWN, null);
        }
        List<RegisteredDevice> active = rows.stream()
                .filter(d -> d.getStatus() == RegistrationStatus.ACTIVE && d.getTbDeviceId() != null)
                .toList();
        if (active.size() > 1) {
            log.warn("[backup-channel] ambiguous ACTIVE registrations for {}: {} rows",
                    devEui, active.size());
            return new MatchOutcome(MatchResult.AMBIGUOUS, null);
        }
        if (active.size() == 1) {
            return new MatchOutcome(MatchResult.MATCH, active.get(0).getTbDeviceId().toString());
        }
        return new MatchOutcome(MatchResult.INACTIVE, null);
    }

    private void rollDay() {
        LocalDate today = LocalDate.now(ZoneId.systemDefault());
        if (!today.equals(stats.day.get())) {
            if (stats.day.compareAndSet(stats.day.get(), today)) {
                stats.todayFrames.set(0);
            }
        }
    }

    public enum MatchResult { MATCH, UNKNOWN, INACTIVE, AMBIGUOUS }

    private record CacheEntry(MatchResult outcome, String tbDeviceId, long expiresAt) {}

    // ------------------------------------------------------------ status

    public boolean isRunning() {
        State s = state;
        return s == State.CONNECTED || s == State.CONNECTING || s == State.DISCONNECTED;
    }

    public Status status() {
        MqttClient current = client;
        return new Status(
                runtimeEnabled,
                state.name(),
                subscribed,
                current != null && current.isConnected(),
                brokerHost, brokerPort,
                properties.getClientId(),
                properties.getTopicFilter(),
                properties.getQos(),
                List.copyOf(whitelist),
                whitelist.isEmpty() ? "COUNT_ONLY" : "WRITE",
                stats.snapshot(),
                stats.startedAt, stats.lastConnectedAt, stats.lastFrameAt, stats.lastWriteAt,
                stats.lastError);
    }

    public record Status(boolean enabled, String state, boolean subscribed, boolean connected,
                         String brokerHost, int brokerPort, String clientId, String topicFilter,
                         int qos, List<Long> whitelist, String mode, Stats.Snapshot counters,
                         Instant startedAt, Instant lastConnectedAt, Instant lastFrameAt,
                         Instant lastWriteAt, String lastError) {}

    /** Every discard path is counted — the channel must not become a silent dropper. */
    public static final class Stats {
        public final AtomicLong received = new AtomicLong();
        public final AtomicLong parseFailed = new AtomicLong();
        public final AtomicLong otherIgnored = new AtomicLong();
        public final AtomicLong notWhitelisted = new AtomicLong();
        public final AtomicLong projectMismatch = new AtomicLong();
        public final AtomicLong unknownDevice = new AtomicLong();
        public final AtomicLong inactiveDevice = new AtomicLong();
        public final AtomicLong ambiguousMatch = new AtomicLong();
        public final AtomicLong matched = new AtomicLong();
        public final AtomicLong written = new AtomicLong();
        public final AtomicLong writeFailed = new AtomicLong();
        public final AtomicLong queueOverflow = new AtomicLong();
        public final AtomicLong connectionAttempts = new AtomicLong();
        public final AtomicLong connectionLosts = new AtomicLong();
        public final AtomicLong todayFrames = new AtomicLong();
        public final AtomicReference<LocalDate> day =
                new AtomicReference<>(LocalDate.now(ZoneId.systemDefault()));
        public volatile Instant startedAt;
        public volatile Instant lastConnectedAt;
        public volatile Instant lastFrameAt;
        public volatile Instant lastWriteAt;
        public volatile String lastError;

        public Snapshot snapshot() {
            return new Snapshot(
                    received.get(), parseFailed.get(), otherIgnored.get(), notWhitelisted.get(),
                    projectMismatch.get(), unknownDevice.get(), inactiveDevice.get(),
                    ambiguousMatch.get(), matched.get(), written.get(), writeFailed.get(),
                    queueOverflow.get(), connectionAttempts.get(), connectionLosts.get(),
                    todayFrames.get());
        }

        public record Snapshot(long received, long parseFailed, long otherIgnored,
                               long notWhitelisted, long projectMismatch, long unknownDevice,
                               long inactiveDevice, long ambiguousMatch, long matched,
                               long written, long writeFailed, long queueOverflow,
                               long connectionAttempts, long connectionLosts, long todayFrames) {}
    }
}
