//process.on('hello',function() {
//    console.log('world!!')
//})


function sayHello()
{
  console.log('HEY TICK !!')
}

for(i=0; i<4; i++)
  console.log('HEY FROM NODE !!');


setInterval(sayHello,1000)
setTimeout(sayHello, 1000)


console.log('Ticking...')

