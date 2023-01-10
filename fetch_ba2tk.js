const cp = require('child_process');

async function main() {
  try {
    await fs.stat(bsatk);
    await cp.spawn('git', ['update'], { cwd: 'ba2tk' });
  } catch (err) {
    await cp.spawn('git', ['clone', '--branch', 'master', '--depth=1', 'https://github.com/Nexus-Mods/ba2tk', 'ba2tk']);
  }
}

main();
