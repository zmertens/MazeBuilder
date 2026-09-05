#!/usr/bin/env node
'use strict';

const exportedModule = require('./mazebuildercli.js');
const ModuleFactory = exportedModule.default || exportedModule;

const loadModule = async() => {
    const activeModule = await ModuleFactory();
    return await activeModule.get();
};

let cli = null;

loadModule().then(module => cli = module);

function normalizeText(value) {
  return String(value ?? '').replace(/\r/g, '').trim();
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

  if (text.toLowerCase().includes('maze generated') || text.toLowerCase().includes('masked maze generated')) {
    return;
  }

  if (!(looksLikeMaze(text) || looksLikeJson(text))) {
    throw new Error(`Unexpected CLI output, not maze text or JSON: ${text.slice(0, 200)}`);
  }
}

async function main() {
  const args = process.argv.slice(2);

  if (!cli) {
    cli = await loadModule();
  }

  if (!cli) {
    throw new Error('Failed to load the WASM module.');
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
