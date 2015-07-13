scene.on('resize', function(width, height) {
    console.log('resize:' + width + ' height:' + height);
});

scene.on('keydown', function(code, flags) {
    console.log("keydown:" + code);
});

function testScene() 
{
    var root = scene.root;

//    var url  = process.cwd() + "/../../images/ball.png"
//    var bg = scene.createImage({id:"bg",url:url,xStretch:2,yStretch:2,parent:root,x:250,y:100});

//    var url2 = process.cwd() + "/../../images/ball3.png"
   // var bg2 = scene.createImage({id:"bg2",url:url2,xStretch:1,yStretch:1,parent:root,x:200,y:100});

//    var frame = scene.createRectangle({fillColor:0x00FF00ff,lineColor:0xFF0000ff,lineWidth:10, x:100, y:5, w:50, h:100, parent:root});
//   var rectangle = scene.createRectangle({fillColor:0xFF0000ff,x:10, y:150, w:50, h:200, parent:root});

    var slice9url = process.cwd() + "/../../images/curve_rectangle.png"
    var slice9 = scene.createImage9({url:slice9url, x:100, y:100, x1:65, y1:65, x2:190, y1:190, w:300, h:200, parent:root});

}

scene.on("keydown", function(code, flags) {
    console.log("keydown in child", code, flags);
});

testScene();

