const process = require('process');
const os = require('os');
const fs = require('fs');
const { execSync } = require('child_process');
const { version } = require('./package.json');

const isFreeBSD = os.platform() === 'freebsd';
const isWindows = os.platform() === 'win32';
// const isMacos = os.platform() === 'darwin';
// const isLinux = os.platform() === 'linux';
const task = process.argv.slice(2).join(' ');
const jobs = Math.ceil(Object.keys(os.cpus()).length / 2);

// mediasoup mayor version.
const MAYOR_VERSION = version.split('.')[0];

// make command to use.
const MAKE = process.env.MAKE || (isFreeBSD ? 'gmake' : 'make');

// eslint-disable-next-line no-console
console.log(`npm-scripts.js [INFO] running task "${task}"`);

switch (task)
{
	case 'typescript:build':
	{
		if (!isWindows)
		{
			execute('rm -rf node/dist');
		}
		else
		{
			execute('rmdir /s /q node/dist');
		}

		execute('tsc --project node');
		// taskReplaceVersion();

		break;
	}
	case 'nginx:build':
	{
    if (isWindows) {
      console.log("please unzip openssl-1.1.1l.tar.gz manually");
    } else {
      execute(`cd nginx/dep && tar -zxvf openssl-1.1.1l.tar.gz `);
    }
		execute(`cd nginx && ./configure --with-stream --with-stream_ssl_module --with-openssl=./dep/openssl-1.1.1l --prefix=/app/cst-nginx && make -j${jobs}`);
		break;
	}
	case 'postinstall':
	{
    execute('node npm-scripts.js nginx:build');
		break;
	}
  case 'debug':
	{
    execute('node npm-scripts.js typescript:build');
    execute('node node/dist/app.js');
		break;
	}
	default:
	{
		throw new TypeError(`unknown task "${task}"`);
	}
}

function execute(command)
{
	// eslint-disable-next-line no-console
	console.log(`npm-scripts.js [INFO] executing command: ${command}`);

	try
	{
		execSync(command, { stdio: [ 'ignore', process.stdout, process.stderr ] });
	}
	catch (error)
	{
		process.exit(1);
	}
}
