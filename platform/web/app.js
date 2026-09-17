'use strict';
/* HKT 负载解码器调试平台 前端逻辑（Vue3 全局构建，无构建步骤） */
const { createApp } = Vue;

createApp({
  data() {
    return {
      devices: [],
      filters: { q: '', origin: '', platform: '', status: '' },
      currentId: null,
      dev: null,
      tab: 'codecs',
      bench: { file: '', fPort: '', timeoutMs: '', text: '', running: false, results: [], summary: null },
      reg: { running: false, cases: [], summary: null },
      diff: { fileA: '', fileB: '', fPort: '', text: '', running: false, results: [], summary: null },
      codeView: { open: false, file: '', content: '' },
      syncing: false,
      toast: null,
    };
  },
  computed: {
    filteredDevices() {
      const q = this.filters.q.toLowerCase();
      return this.devices.filter(d => {
        if (this.filters.origin && d.origin !== this.filters.origin) return false;
        if (this.filters.platform && !d.stats.platforms.includes(this.filters.platform)) return false;
        if (this.filters.status === 'verified' && !d.stats.verifiedCount) return false;
        if (this.filters.status === 'unverified' && !d.stats.unverifiedCount) return false;
        if (this.filters.status === 'missing' && !d.stats.missingAuthority) return false;
        if (q) {
          const hay = [d.model, d.name, d.vendor, d.category, ...(d.aliases || [])].join(' ').toLowerCase();
          if (!hay.includes(q)) return false;
        }
        return true;
      });
    },
  },
  methods: {
    async loadDevices() {
      const r = await fetch('/api/devices').then(r => r.json());
      this.devices = r.devices || [];
    },
    async openDevice(id) {
      this.currentId = id;
      this.dev = await fetch('/api/device?id=' + encodeURIComponent(id)).then(r => r.json());
      this.tab = 'codecs';
      this.bench = { file: '', fPort: this.dev.fPortDefault == null ? '' : String(this.dev.fPortDefault), timeoutMs: '', text: '', running: false, results: [], summary: null };
      this.reg = { running: false, cases: [], summary: null };
      this.diff = { fileA: '', fileB: '', fPort: '', text: '', running: false, results: [], summary: null };
      const current = this.dev.codecs.filter(c => c.current);
      if (current.length) this.bench.file = current[0].file;
      if (this.dev.codecs.length) {
        this.diff.fileA = (current[0] || this.dev.codecs[0]).file;
        const other = (current[1] || this.dev.codecs.find(c => c.file !== this.diff.fileA));
        this.diff.fileB = other ? other.file : this.diff.fileA;
      }
      history.replaceState(null, '', '#' + encodeURIComponent(id));
    },
    platformLabel(p) { return { chirpstack: 'ChirpStack', ttn: 'TTN', thingsboard: 'ThingsBoard' }[p] || p; },
    shortFile(f) { return String(f || '').replace(/_[^_]*\.js$/, '').replace(/_/g, ' ') || f; },
    fmtJson(v) { return v == null ? '—' : JSON.stringify(v, null, 2); },
    devicePath(file) { return 'decoders/' + this.currentId + '/' + file; },
    parseBenchLines(text) {
      const out = [];
      for (let line of String(text || '').split(/\r?\n/)) {
        line = line.trim();
        if (!line || line.startsWith('#') || line.startsWith('//')) continue;
        let fPort = null;
        const m = line.match(/^\[fPort\s*=\s*(\d+)\]\s*(.*)$/i);
        if (m) { fPort = Number(m[1]); line = m[2].trim(); }
        out.push({ raw: line, fPort });
      }
      return out;
    },
    useInBench(c) { this.tab = 'bench'; this.bench.file = c.file; },
    async runBench() {
      const payloads = this.parseBenchLines(this.bench.text);
      if (!payloads.length) return this.showToast('请先粘贴至少一条负载', 'bad');
      this.bench.running = true; this.bench.results = []; this.bench.summary = null;
      try {
        const r = await fetch('/api/decode', {
          method: 'POST', headers: { 'content-type': 'application/json' },
          body: JSON.stringify({ device: this.currentId, file: this.bench.file, payloads, fPort: this.bench.fPort === '' ? null : Number(this.bench.fPort), timeoutMs: Number(this.bench.timeoutMs) || undefined }),
        }).then(r => r.json());
        if (r.error) this.showToast(r.error, 'bad');
        else { this.bench.results = r.results; this.bench.summary = r.summary; }
      } catch (e) { this.showToast('请求失败: ' + e.message, 'bad'); }
      this.bench.running = false;
    },
    loadSamplePayloads() {
      if (!this.dev.samples.length) return this.showToast('该设备暂无黄金样例', 'bad');
      const s = this.dev.samples;
      this.bench.text = s.map(x => (x.fPort != null ? '[fPort=' + x.fPort + '] ' : '') + x.payload).join('\n');
      const fp = s.find(x => x.fPort != null);
      if (fp && this.bench.fPort === '') this.bench.fPort = String(fp.fPort);
    },
    async runRegression(file) {
      this.reg.running = true; this.reg.cases = []; this.reg.summary = null;
      try {
        const r = await fetch('/api/regression', {
          method: 'POST', headers: { 'content-type': 'application/json' },
          body: JSON.stringify({ device: this.currentId, file: file || undefined }),
        }).then(r => r.json());
        this.reg.cases = r.cases || []; this.reg.summary = r.summary || null;
      } catch (e) { this.showToast('请求失败: ' + e.message, 'bad'); }
      this.reg.running = false;
    },
    async runDiff() {
      const payloads = this.parseBenchLines(this.diff.text);
      if (!payloads.length) return this.showToast('请先粘贴至少一条负载', 'bad');
      if (this.diff.fileA === this.diff.fileB) return this.showToast('两个版本相同，无需对比', 'bad');
      this.diff.running = true; this.diff.results = []; this.diff.summary = null;
      try {
        const r = await fetch('/api/diff', {
          method: 'POST', headers: { 'content-type': 'application/json' },
          body: JSON.stringify({ device: this.currentId, fileA: this.diff.fileA, fileB: this.diff.fileB, payloads, fPort: this.diff.fPort === '' ? null : Number(this.diff.fPort) }),
        }).then(r => r.json());
        if (r.error) this.showToast(r.error, 'bad');
        else { this.diff.results = r.results; this.diff.summary = r.summary; }
      } catch (e) { this.showToast('请求失败: ' + e.message, 'bad'); }
      this.diff.running = false;
    },
    async viewCode(file) {
      const r = await fetch('/api/file?path=' + encodeURIComponent(this.devicePath(file))).then(r => r.json());
      if (r.error) return this.showToast(r.error, 'bad');
      this.codeView = { open: true, file, content: r.content };
    },
    async copyCode(file) {
      const r = await fetch('/api/file?path=' + encodeURIComponent(this.devicePath(file))).then(r => r.json());
      if (r.error) return this.showToast(r.error, 'bad');
      try { await navigator.clipboard.writeText(r.content); this.showToast('已复制 ' + file); }
      catch (e) { this.showToast('复制失败（浏览器权限）', 'bad'); }
    },
    async copyViewed() {
      try { await navigator.clipboard.writeText(this.codeView.content); this.showToast('已复制'); }
      catch (e) { this.showToast('复制失败（浏览器权限）', 'bad'); }
    },
    openDoc(file) { window.open('/api/doc?path=' + encodeURIComponent(this.devicePath(file)), '_blank'); },
    async doSync() {
      this.syncing = true;
      try {
        const r = await fetch('/api/sync', { method: 'POST' }).then(r => r.json());
        this.showToast(r.ok ? '同步完成' : '同步失败', r.ok ? 'ok' : 'bad');
        if (!r.ok && r.error) console.warn(r.error, r.output);
        await this.loadDevices();
        if (this.currentId) await this.openDevice(this.currentId);
      } catch (e) { this.showToast('同步请求失败: ' + e.message, 'bad'); }
      this.syncing = false;
    },
    showToast(text, type) {
      this.toast = { text, type: type || 'ok' };
      clearTimeout(this._tt);
      this._tt = setTimeout(() => { this.toast = null; }, 3000);
    },
  },
  async mounted() {
    await this.loadDevices();
    const hash = decodeURIComponent(location.hash.replace(/^#/, ''));
    if (hash && this.devices.some(d => d.id === hash)) this.openDevice(hash);
  },
}).mount('#app');
