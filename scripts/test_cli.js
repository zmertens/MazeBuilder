#!/usr/bin/env node
'use strict';

const fs = require('node:fs');
const path = require('node:path');
const { pathToFileURL } = require('node:url');

function normalizeText(value) {
  return String(value ?? '').replace(/\r/g, '').trim();
}

function findWasmModulePath(startDir) {
  const roots = new Set(
    [process.cwd(), startDir, path.resolve(startDir, '..')].filter(Boolean)
  );

  for (const root of roots) {
    const candidates = [
      path.join(root, 'mazebuildercli.js')
    ];

    const found = candidates.find((candidate) => fs.existsSync(candidate));
    if (found) {
      return found;
    }
  }

  return null;
}

function findWasmBinary(modulePath) {
  const dir = path.dirname(modulePath);
  const candidates = [
    path.join(dir, 'mazebuildercli.wasm')
  ];

  const direct = candidates.find((candidate) => fs.existsSync(candidate));
  if (direct) {
    return direct;
  }
  return null;
}

function looksLikeHelp(text) {
  return /(?:^|\s)v\d+\.\d+\.\d+|--help|-h|--version|-v|usage|commands are case-sensitive/i.test(text);
}

function looksLikeMaze(text) {
  return /\+[-+]+\+/.test(text) && /\|/.test(text);
}

function looksLikeJson(text) {
  return /^[\[{]/.test(text.trim());
}

function validateOutput(args, output) {
  const text = normalizeText(output);
  if (!text) {
    throw new Error('CLI returned empty output.');
  }

  const isHelpRequest =
    args.length === 0 ||
    args.includes('-h') ||
    args.includes('--help') ||
    args.includes('-v') ||
    args.includes('--version');

  if (isHelpRequest) {
    if (!looksLikeHelp(text)) {
      throw new Error(`Expected help/version output, received: ${text.slice(0, 160)}`);
    }
    return;
  }

  if (text.toLowerCase().includes('error') || text.toLowerCase().includes('failed')) {
    throw new Error(`Unexpected CLI error: ${text.slice(0, 200)}`);
  }

  if (!(looksLikeMaze(text) || looksLikeJson(text))) {
    throw new Error(`Unexpected CLI output, not maze text or JSON: ${text.slice(0, 200)}`);
  }
}

async function main() {
  const args = process.argv.slice(2);
  const modulePath = findWasmModulePath(__dirname);

  if (!modulePath) {
    throw new Error('Could not find the generated mazebuildercli.js bundle. Build the Emscripten CLI target first.');
  }

  const wasmPath = findWasmBinary(modulePath);
  if (!wasmPath) {
    throw new Error(`Found ${modulePath}, but no matching .wasm file was found nearby. Build the Emscripten CLI target so the bundle is generated.`);
  }

  const imported = await import(pathToFileURL(modulePath).href);
  const factory = imported.default || imported;

  const instance = await factory({
    print: (message = '') => {
      process.stdout.write(`${String(message)}\n`);
    },
    printErr: (message = '') => {
      process.stderr.write(`${String(message)}\n`);
    },
    locateFile: (filename) => {
      if (filename && /\.wasm$/i.test(filename)) {
        return wasmPath;
      }
      return path.resolve(path.dirname(modulePath), filename);
    },
  });

  if (!instance || typeof instance.get !== 'function') {
    throw new Error('WASM module did not expose the expected get() API.');
  }

  const cli = instance.get();
  if (!cli || typeof cli.run !== 'function') {
    throw new Error('WASM module did not return a usable CLI instance.');
  }

  const commandString = args.join(' ');
  let output;

  if (args.length === 0 || args.includes('-h') || args.includes('--help')) {
    output = cli.help();
  } else if (args.includes('-v') || args.includes('--version')) {
    output = cli.version();
  } else {
    output = cli.run(commandString);
  }

  validateOutput(args, output);
  process.stdout.write(`${normalizeText(output)}\n`);
}

main().catch((error) => {
  const message = error && error.message ? error.message : String(error);
  console.error(`mazebuildercli test failed: ${message}`);
  process.exit(1);
});
