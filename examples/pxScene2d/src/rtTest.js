//var px = require("./jsbindings/build/Debug/px");

//process.on('hello',function() {
//    console.log('world!!')
//})

debugger;

//var scene = px.getScene(0, 0, 800, 400);
//var scene = px.getScene(0, 0, ww, eh);

//myRect.x = 0
var total = 0

function sayHello()
{

  console.log('HEY TICK !!')
    total++

 //   myRect.x+=5
}

//for(i=0; i<4; i++)
//  console.log('HEY FROM NODE !!');


setInterval(sayHello,1000)
setTimeout(sayHello, 1000)


console.log('Ticking...')

