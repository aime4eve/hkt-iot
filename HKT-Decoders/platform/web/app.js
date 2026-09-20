'use strict';
/* HKT 负载解码器调试平台 前端逻辑（Vue3 全局构建，无构建步骤） */
const { createApp } = Vue;

// 前端错误可视化：任何未捕获错误直接显示为页面顶部红色横幅（便于用户截图反馈）
window.APP_VER = '20260917d';
function showFatalBanner(msg) {
  let bar = document.getElementById('fatal-banner');
  if (!bar) {
    bar = document.createElement('div');
    bar.id = 'fatal-banner';
    bar.style.cssText = 'position:fixed;top:0;left:0;right:0;z-index:9999;background:#c0392b;color:#fff;padding:10px 16px;font:13px/1.5 monospace;white-space:pre-wrap;word-break:break-all;max-height:40vh;overflow:auto;';
    document.body.appendChild(bar);
  }
  bar.textContent = '⚠ 前端错误 [v' + window.APP_VER + '] ' + msg;
}
window.addEventListener('error', e => showFatalBanner(e.message + ' @ ' + String(e.filename || '').split('/').pop() + ':' + e.lineno));
window.addEventListener('unhandledrejection', e => showFatalBanner(String((e.reason && (e.reason.stack || e.reason.message)) || e.reason)));

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
      importing: false,
      newDev: { open: false, saving: false, err: '', form: this.blankNewDev() },
      mgmt: { saving: false, form: {}, nc: { platform: 'chirpstack', lang: '', version: '', firmwareVersion: '', changelog: '', code: '' }, samplesText: '[]' },
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
    blankNewDev() {
      return { model: '', modelKey: '', name: '', origin: 'out-sourced', vendor: '', vendorKey: '', category: '', fPortDefault: '', aliases: '', authType: 'protocol-doc', authPath: '', authRef: '', authSection: '', authMissing: false };
    },
    modelKeyBad(s) { return !/^[a-z0-9][a-z0-9-]*$/.test(s || ''); },
    onOriginChange() {
      this.newDev.form.authType = this.newDev.form.origin === 'in-house' ? 'firmware' : 'protocol-doc';
    },
    async loadDevices() {
      const r = await fetch('/api/devices').then(r => r.json()).catch(e => ({ error: e.message }));
      if (r.error) return this.showToast('设备列表加载失败: ' + r.error, 'bad');
      this.devices = r.devices || [];
    },
    async openDevice(id) {
      this.currentId = id;
      const data = await fetch('/api/device?id=' + encodeURIComponent(id)).then(r => r.json()).catch(e => ({ error: e.message }));
      if (data.error) { this.showToast('设备详情加载失败: ' + data.error, 'bad'); this.currentId = null; return; }
      this.dev = data;
      // 切换设备后右侧内容区滚回自己的顶部（左右分屏，互不影响左侧列表）
      var mainEl = document.querySelector('.main');
      if (mainEl) mainEl.scrollTop = 0;
      this.tab = 'codecs';
      this.bench = { file: '', fPort: this.dev.fPortDefault == null ? '' : String(this.dev.fPortDefault), timeoutMs: '', text: '', running: false, results: [], summary: null };
      this.reg = { running: false, cases: [], summary: null };
      this.diff = { fileA: '', fileB: '', fPort: '', text: '', running: false, results: [], summary: null };
      this.fillMgmtForm();
      const current = this.dev.codecs.filter(c => c.current);
      this.bench.file = current.length ? current[0].file : (this.dev.codecs[0] ? this.dev.codecs[0].file : '');
      if (this.dev.codecs.length) {
        this.diff.fileA = (current[0] || this.dev.codecs[0]).file;
        const other = (current[1] || this.dev.codecs.find(c => c.file !== this.diff.fileA));
        this.diff.fileB = other ? other.file : this.diff.fileA;
      } else {
        this.diff.fileA = this.diff.fileB = '';
      }
      history.replaceState(null, '', '#' + encodeURIComponent(id));
    },
    platformLabel(p) { return { chirpstack: 'ChirpStack', ttn: 'TTN', thingsboard: 'ThingsBoard', private: '私有协议' }[p] || p; },
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
    exportBackup() { window.open('/api/export', '_blank'); this.showToast('备份包开始下载'); },
    importClick() { this.$refs.importFile.click(); },
    async doImport(ev) {
      const f = ev.target.files[0];
      ev.target.value = '';
      if (!f) return;
      this.importing = true;
      try {
        const fd = new FormData();
        fd.append('file', f);
        const r = await fetch('/api/import', { method: 'POST', body: fd }).then(r => r.json());
        if (r.error) this.showToast(r.error, 'bad');
        else {
          const created = r.devices.filter(d => d.action === 'created').length;
          this.showToast(`导入完成：新增 ${created} 台设备，合并 ${r.devices.length - created} 台，写入 ${r.filesWritten} 个文件${r.skipped.length ? '，跳过 ' + r.skipped.length : ''}`, 'ok');
          await this.loadDevices();
        }
      } catch (e) { this.showToast('导入失败: ' + e.message, 'bad'); }
      this.importing = false;
    },
    async createDevice() {
      const f = this.newDev.form;
      this.newDev.err = '';
      if (!f.model.trim() || !f.name.trim()) { this.newDev.err = '型号 / 名称必填'; return; }
      if (!/^[a-z0-9][a-z0-9-]*$/.test(f.modelKey || '')) {
        this.newDev.err = 'modelKey 需为小写字母/数字/连字符，且以字母或数字开头（如 gm-200）';
        return;
      }
      if (f.origin === 'out-sourced' && !f.vendorKey) { this.newDev.err = '采购设备需填厂商目录名（如 oufu）'; return; }
      this.newDev.saving = true;
      try {
        const body = {
          model: f.model, modelKey: f.modelKey, name: f.name, origin: f.origin,
          vendor: f.vendor, vendorKey: f.vendorKey, category: f.category, fPortDefault: f.fPortDefault,
          aliases: f.aliases ? f.aliases.split(/[,，\s]+/).filter(Boolean) : [],
          // 创建时文档必然尚未进平台：一律 missing=true（上传后 saveDoc 自动转正），
          // note 区分「已有文档建后上传」与「暂缺记债务」，满足服务端 accountability 校验
          authority: f.authType === 'firmware'
            ? { type: 'firmware', path: f.authPath, ref: f.authRef }
            : { type: 'protocol-doc', file: null, section: f.authSection,
                missing: true, note: f.authMissing ? '暂缺协议文档（债务台账）' : '已有文档，创建后即上传' },
        };
        const r = await fetch('/api/devices', { method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify(body) }).then(r => r.json());
        if (r.error) { this.newDev.err = r.error; return; }
        this.showToast('设备已创建: ' + r.id);
        this.newDev.open = false;
        this.newDev.form = this.blankNewDev();
        await this.loadDevices();
        await this.openDevice(r.id);
        this.tab = 'manage';
      } catch (e) { this.newDev.err = '创建失败: ' + e.message; }
      finally { this.newDev.saving = false; }
    },
    fillMgmtForm() {
      const d = this.dev;
      this.mgmt.form = {
        name: d.name, model: d.model, vendor: d.vendor, category: d.category,
        aliases: (d.aliases || []).join(', '), notes: d.notes || '',
        fPortDefault: d.fPortDefault == null ? '' : String(d.fPortDefault),
        authType: d.authority.type, authPath: d.authority.path || '', authRef: d.authority.ref || '',
        authFile: d.authority.file || '', authSection: d.authority.section || '', authMissing: !!d.authority.missing,
      };
      this.mgmt.nc = { platform: 'chirpstack', lang: '', version: '', firmwareVersion: '', changelog: '', code: '' };
      this.mgmt.samplesText = JSON.stringify(this.dev.samples || [], null, 2);
    },
    async saveMeta() {
      this.mgmt.saving = true;
      try {
        const f = this.mgmt.form;
        const r = await fetch('/api/device', {
          method: 'PATCH', headers: { 'content-type': 'application/json' },
          body: JSON.stringify({
            id: this.currentId,
            patch: {
              name: f.name, model: f.model, vendor: f.vendor, category: f.category,
              aliases: f.aliases, notes: f.notes, fPortDefault: f.fPortDefault,
              authority: f.authType === 'firmware'
                ? { type: 'firmware', path: f.authPath, ref: f.authRef }
                : { type: 'protocol-doc', file: f.authFile, section: f.authSection },
            },
          }),
        }).then(r => r.json());
        if (r.error) return this.showToast(r.error, 'bad');
        this.showToast('台账已保存');
        await this.openDevice(this.currentId); this.tab = 'manage';
      } catch (e) { this.showToast('保存失败: ' + e.message, 'bad'); }
      finally { this.mgmt.saving = false; }
    },
    mgmtFilePicked(ev) {
      const f = ev.target.files[0];
      ev.target.value = '';
      if (!f) return;
      const reader = new FileReader();
      reader.onload = () => {
        this.mgmt.nc.code = String(reader.result);
        const m = f.name.match(/_v(\d+\.\d+\.\d+)\.js$/);
        if (m && !this.mgmt.nc.version) this.mgmt.nc.version = m[1];
        const pm = f.name.match(/_(chirpstack|ttn|thingsboard)/);
        if (pm) this.mgmt.nc.platform = pm[1];
      };
      reader.readAsText(f);
    },
    async addCodec() {
      this.mgmt.saving = true;
      try {
        const nc = this.mgmt.nc;
        const r = await fetch('/api/device/codecs', {
          method: 'POST', headers: { 'content-type': 'application/json' },
          body: JSON.stringify({ id: this.currentId, platform: nc.platform, lang: nc.lang || null, version: nc.version, firmwareVersion: nc.firmwareVersion, changelog: nc.changelog, code: nc.code }),
        }).then(r => r.json());
        if (r.error) return this.showToast(r.error, 'bad');
        this.showToast('新版本已保存: ' + r.file);
        await this.openDevice(this.currentId); this.tab = 'manage';
      } catch (e) { this.showToast('保存失败: ' + e.message, 'bad'); }
      finally { this.mgmt.saving = false; }
    },
    async setCurrent(file) {
      const r = await fetch('/api/device/codec-current', { method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify({ id: this.currentId, file }) }).then(r => r.json());
      if (r.error) return this.showToast(r.error, 'bad');
      this.showToast('current 已切换: ' + file);
      await this.openDevice(this.currentId); this.tab = 'codecs';
    },
    async verifyClick(c) {
      const basis = prompt('填写核验依据（每行一条）\n例如：\n手册 v1.2 §5 通道表\n黄金样例全部通过');
      if (basis === null) return;
      const arr = basis.split(/\r?\n/).map(s => s.trim()).filter(Boolean);
      const r = await fetch('/api/device/codec-status', { method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify({ id: this.currentId, file: c.file, status: 'verified', basis: arr }) }).then(r => r.json());
      if (r.error) return this.showToast(r.error, 'bad');
      this.showToast('已标记为已核验: ' + c.file);
      await this.openDevice(this.currentId); this.tab = 'codecs';
    },
    async deleteCodec(c) {
      const msg = c.current
        ? '⚠ ' + c.file + ' 是 current 版本！\n\n删除后该平台' + (c.lang ? '/' + c.lang.toUpperCase() : '') + '将没有任何可用版本，\n绑定它的样例将回退为按全部 current 回归。\n\n文件会移入服务器回收站（不直接销毁）。\n\n确认删除？'
        : '确认删除 ' + c.file + ' ？\n（文件会移入服务器回收站，不直接销毁）';
      if (!confirm(msg)) return;
      const r = await fetch('/api/device/codec', { method: 'DELETE', headers: { 'content-type': 'application/json' }, body: JSON.stringify({ id: this.currentId, file: c.file }) }).then(r => r.json());
      if (r.error) return this.showToast(r.error, 'bad');
      this.showToast('已删除: ' + c.file);
      await this.openDevice(this.currentId); this.tab = 'codecs';
    },
    async saveSamples() {
      let samples;
      try { samples = JSON.parse(this.mgmt.samplesText); } catch (e) { return this.showToast('JSON 格式错误: ' + e.message, 'bad'); }
      this.mgmt.saving = true;
      try {
        const r = await fetch('/api/device/samples', { method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify({ id: this.currentId, samples }) }).then(r => r.json());
        if (r.error) return this.showToast(r.error, 'bad');
        this.showToast('样例已保存（' + r.count + ' 条）');
        await this.openDevice(this.currentId); this.tab = 'manage';
      } catch (e) { this.showToast('保存失败: ' + e.message, 'bad'); }
      finally { this.mgmt.saving = false; }
    },
    async uploadDoc(ev) {
      const f = ev.target.files[0];
      ev.target.value = '';
      if (!f) return;
      const fd = new FormData();
      fd.append('file', f);
      const r = await fetch('/api/device/docs?id=' + encodeURIComponent(this.currentId), { method: 'POST', body: fd }).then(r => r.json());
      if (r.error) return this.showToast(r.error, 'bad');
      this.showToast('文档已上传: ' + r.file);
      await this.openDevice(this.currentId); this.tab = 'manage';
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
