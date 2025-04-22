const fs = require('fs');

const opcodes = fs.readFileSync('../../../common/emu_oplist.h').toString().split('\n');
const protoLines = fs.readFileSync('../EQMessage.proto').toString().split('\n');

let opIdx = 1;
const opMap = {};
for (const line of opcodes) {
	if (!line.trim().startsWith('N(OP_')) {
		continue;
	}
	const [,opCode] = /N\((\w+)\)/.exec(line);
	opMap[opCode] = opIdx;
	opIdx++
}


for (const [idx, protoLine] of Object.entries(protoLines)) {
	for (const [opCode, val] of Object.entries(opMap)) {
		if (protoLine.trim().startsWith(opCode + ' ')) {
			const newLine = protoLine.replace(/= \d+/, `= ${val}`);
			protoLines[idx] = newLine;
			break;
		}
	}
}


fs.writeFileSync('../EQMessage.proto', protoLines.join('\n'));