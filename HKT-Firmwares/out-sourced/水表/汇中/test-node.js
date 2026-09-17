const fs = require("fs");
const path = require("path");
const decoder = require("./uni-decode-en.js");

function hexToBytes(hex) {
  const clean = String(hex || "").trim().replace(/\s/g, "");
  if (!clean) return [];
  if (clean.length % 2 !== 0) {
    throw new Error(`Invalid hex length: ${clean.length}`);
  }
  const bytes = [];
  for (let i = 0; i < clean.length; i += 2) {
    bytes.push(parseInt(clean.substring(i, i + 2), 16));
  }
  return bytes;
}

function loadTestData(filePath) {
  return fs
    .readFileSync(filePath, "utf8")
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter(Boolean);
}

function run() {
  const inputFile = path.join(__dirname, "encode-test.txt");
  const outputFile = path.join(__dirname, "test-result.txt");
  const testData = loadTestData(inputFile);
  const outputLines = [];
  const print = (line = "") => {
    console.log(line);
    outputLines.push(line);
  };
  print("=".repeat(70));
  print("Test decodeUplink with encode-test.txt");
  print("=".repeat(70));
  print(`Input file: ${inputFile}`);
  print(`Cases: ${testData.length}`);

  testData.forEach((hex, index) => {
    print(`\n${"-".repeat(70)}`);
    print(`Case #${index + 1}`);
    print(`Payload: ${hex}`);
    try {
      const bytes = hexToBytes(hex);
      const result = decoder.decodeUplink({ bytes });
      const data = result?.data || {};
      if (data.error) {
        print(`Result: ERROR | ${data.error}`);
        return;
      }
      print(
        `Result: OK | afn=${data.afn} | afnText=${data.afnText} | alarmCount=${data.alarmCount} | dataBlocks=${data.dataBlocks}`
      );
      const warningKeys = Object.keys(data).filter((k) => /^warning\d*$/.test(k));
      if (warningKeys.length === 0) {
        print("Warnings: none");
      } else {
        warningKeys.forEach((k) => print(`Warning: ${data[k]}`));
      }
      const dataJson = JSON.stringify(data, null, 2);
      print("Data JSON:");
      console.log(dataJson);
      outputLines.push(dataJson);
    } catch (error) {
      print(`Result: EXCEPTION | ${error.message}`);
    }
  });
  fs.writeFileSync(outputFile, `${outputLines.join("\n")}\n`, "utf8");
  console.log(`\nSaved test result to: ${outputFile}`);
}

run();
