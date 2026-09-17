/**
 * 读取 sn.txt，调用 decodeUplink({ bytes })，将返回值原样 JSON 写入 sn-decoded.json
 * 用法: node run-decode-sn.js
 */

const fs = require("fs");
const path = require("path");
const { decodeUplink } = require("./gm100-decode-cn.js");

const dir = __dirname;
const hexPath = path.join(dir, "sn.txt");
const outPath = path.join(dir, "sn-decoded.json");

const hex = fs.readFileSync(hexPath, "utf8").trim().replace(/\s/g, "");
if (!hex || hex.length % 2 !== 0) {
  throw new Error(`sn.txt 无效或长度非偶数字符: ${hexPath}`);
}

const bytes = [];
for (let i = 0; i < hex.length; i += 2) {
  bytes.push(parseInt(hex.substr(i, 2), 16));
}

const result = decodeUplink({ bytes });
fs.writeFileSync(outPath, JSON.stringify(result, null, 2), "utf8");
console.log(`已写入 ${outPath}`);
