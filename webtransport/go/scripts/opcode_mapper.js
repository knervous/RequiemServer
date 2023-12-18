const fs = require('fs');

let structFile = fs.readFileSync('../../../common/patches/web_structs.h').toString();
const split = structFile.split('\n')

let inStruct = false;
let currStruct = {
    name: '',
    fields: []
};

const structs = [];

const splitCaps = string => string
    .replace(/([a-z])([A-Z]+)/g, (m, s1, s2) => s1 + ' ' + s2)
    .replace(/([A-Z])([A-Z]+)([^a-zA-Z0-9]*)$/, (m, s1, s2, s3) => s1 + s2.toLowerCase() + s3)
    .replace(/([A-Z]+)([A-Z][a-z])/g,
        (m, s1, s2) => s1.toLowerCase() + ' ' + s2);
const snakeCase = string =>
    splitCaps(string)
        .replace(/\W+/g, " ")
        .split(/ |\B(?=[A-Z])/)
        .map(word => word.toLowerCase())
        .join('_');

for (const [idx, line] of Object.entries(split)) {
    const trimmedLine = line.trim();
    if (trimmedLine === '') {
        continue;
    }
    if (trimmedLine.startsWith('/')) {
        continue;
    }

    if (!inStruct && trimmedLine.startsWith('struct')) {
        inStruct = true;
        currStruct.name = trimmedLine.split(' ')[1];
        continue;
    }

    if (inStruct) {
        if (trimmedLine === '};') {
            inStruct = false;
            structs.push(currStruct);
            currStruct = {
                name: '',
                fields: []
            }
            continue;
        }

        const [content] = trimmedLine.split(';');
        const field = {};
        if (content.startsWith('struct')) {
            let [, structType, name] = content.split(' ');
            const repeated = name.includes('*') || name.includes('[')


            name = name.replace(/[\[\]\d+\*]/g, '');
            split[idx] = line.replace(name, snakeCase(name))

            field.name = name;
            field.type = structType;
            field.repeated = repeated;
            currStruct.fields.push(field)
            continue;
        }

        let [type, name] = content.split(' ');
        if (!name) {
            continue
        }
        if (type.includes('char')) {
            field.type = 'string';
            name = name.replace(/[\[\]\d+\*]/g, '');
            field.name = name;
            split[idx] = line.replace(name, snakeCase(name))
        } else {
            const repeated = name.includes('*') || name.includes('[')
            name = name.replace(/[\[\]\d+\*]/g, '');
            split[idx] = line.replace(name, snakeCase(name))
            field.name = name;
            field.type = type;
            field.repeated = repeated;
        }
        if (JSON.stringify(field) !== '{}') {
            currStruct.fields.push(field)
        }
    }
}

let protomessage = ``;

const getType = type => {
    if (type === 'string') return 'string'
    if (type.includes('int')) return 'int32'
    return type.replace('_Struct', '').replace('enum', '')
}

const opcodes = fs.readFileSync('./opcode_dispatch.h')
    .toString().split('\n').filter(l => l.trim().startsWith('IN') || l.trim().startsWith('OUT'))

const protoOpCodes = fs.readFileSync('../EQMessage.proto').toString().split('\n').map(a => a.trim()).filter(a => a.startsWith('OP_'))
    .map(a => {
        const [op, val] = a.split(' = ');
        return {
            name: op,
            val: +val.replace(';', '')
        }
    })


const opLines = [

]


const findOp = op => {
    const val = protoOpCodes.find(o => o.name === op)
    if (!val) {
        const r = 123;
    }

    return val.val;
}

const inLines = [];
const outLines = []

for (const line of opcodes) {
    const [pref, rest] = line.split('(');
    switch (pref) {
        case 'IN_C':
        case 'INv':
        case 'IN':
            {
                let [op, str] = rest.split(', ');
                str = str.replace(');', '')
                opLines.push(`${op} = ${findOp(op)} [(messageType) = "eq.${str.replace('_Struct', '')}"];`)
                inLines.push(`
                case OpCodes_${op}:
                    return tie(&C.struct_${str}{})`)
                break;
            }
        case 'IN_Cz':
        case 'INz':
            {
                let [op] = rest.split(')');
                opLines.push(`${op} = ${findOp(op)} [(messageType) = "eq.Zero"];`)
                inLines.push(`
                case OpCodes_${op}:
                    return tie(&C.struct_Zero_Struct{})`)
                break;
            }
        case 'OUTz':
            {
                let [op] = rest.split(')');
                opLines.push(`${op} = ${findOp(op)} [(messageType) = "eq.Zero"];`)
                outLines.push(`
                case OpCodes_${op}:
                    return tie((*C.struct_Zero_Struct)(structPtr))
                `)
                break;
            }
        case 'OUT':
        case 'OUTv':
            {
                let [op, str] = rest.split(', ');
                str = str.replace(');', '')
                opLines.push(`${op} = ${findOp(op)} [(messageType) = "eq.${str.replace('_Struct', '')}"];`)
                outLines.push(`
                case OpCodes_${op}:
                    return tie((*C.struct_${str})(structPtr))
                `)
                break;
            }
        default:
            console.log('Unhandled case', pref)
    }
}

fs.writeFileSync('./protomap.proto', opLines.join('\n'))
fs.writeFileSync('./out.txt', outLines.join('\n'))
fs.writeFileSync('./in.txt', inLines.join('\n'))
const r = 123;