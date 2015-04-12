
// A cross-browser requestAnimationFrame
// See https://hacks.mozilla.org/2011/08/animating-with-javascript-from-setinterval-to-requestanimationframe/
var requestAnimFrame = (function(){
    return window.requestAnimationFrame       ||
        window.webkitRequestAnimationFrame ||
        window.mozRequestAnimationFrame    ||
        window.oRequestAnimationFrame      ||
        window.msRequestAnimationFrame     ||
        function(callback){
            window.setTimeout(callback, 1000 / 60);
        };
})();

// Create the canvas
var canvas = document.createElement("canvas");
var ctx = canvas.getContext("2d");
canvas.width = $(window).width() - ($(window).width() % 32);
canvas.height = $(window).height() - ($(window).height() % 32);
document.body.appendChild(canvas);

// The main game loop
var lastTime;
function main() {
    var now = Date.now();
    var dt = (now - lastTime) / 1000.0;

    update(dt);
    render();

    lastTime = now;
    requestAnimFrame(main);
};

function init() {
    terrainPattern = ctx.createPattern(resources.get('img/terrain.png'), 'repeat');
    createTerrain();

    document.getElementById('play-again').addEventListener('click', function() {
        reset();
    });

    reset();
    lastTime = Date.now();
    main();
}

resources.load([
    'img/sprites.png',
    'img/terrain.png',
    'img/onion.png',
    'img/spritesheet.png'
]);
resources.onReady(init);

// Game state
var player = {
    pos: [0, 0],
    sprite: new Sprite('img/onion.png', [0, 0], [32, 32], 16, [0])
};

var bullets = [];
var enemies = [];

var explosions = [];

var lastFire = Date.now();
var gameTime = 0;
var isGameOver;
var terrainPattern;

var score = 0;
var scoreEl = document.getElementById('score');

// Speed in pixels per second
var playerSpeed = 200;
var bulletSpeed = 500;
var enemySpeed = 100;

// Update game objects
function update(dt) {
    gameTime += dt;

    handleInput(dt);
    updateEntities(dt);

    // It gets harder over time by adding enemies using this
    // equation: 1-.993^gameTime
    /*if(Math.random() < 1 - Math.pow(.993, gameTime)) {
        enemies.push({
            pos: [canvas.width,
                  Math.random() * (0)],
            sprite: new Sprite('img/sprites.png', [0, 78], [80, 39],
                               6, [0, 1, 2, 3, 2, 1])
        });
    }*/

    checkCollisions();

    scoreEl.innerHTML = score;
};

function handleInput(dt) {
    if(input.isDown('DOWN') || input.isDown('s')) {
        player.pos[1] += playerSpeed * dt;
    }

    if(input.isDown('UP') || input.isDown('w')) {
        player.pos[1] -= playerSpeed * dt;
    }

    if(input.isDown('LEFT') || input.isDown('a')) {
        player.pos[0] -= playerSpeed * dt;
    }

    if(input.isDown('RIGHT') || input.isDown('d')) {
        player.pos[0] += playerSpeed * dt;
    }

    //console.log("Player x=" + player.pos[0] + ", y=" + player.pos[1]);

    if(input.isDown('SPACE') &&
       !isGameOver &&
       Date.now() - lastFire > 100) {
        var x = player.pos[0] + player.sprite.size[0] / 2;
        var y = player.pos[1] + player.sprite.size[1] / 2;

        bullets.push({ pos: [x, y],
                       dir: 'forward',
                       sprite: new Sprite('img/sprites.png', [0, 39], [18, 8]) });
        bullets.push({ pos: [x, y],
                       dir: 'up',
                       sprite: new Sprite('img/sprites.png', [0, 50], [9, 5]) });
        bullets.push({ pos: [x, y],
                       dir: 'down',
                       sprite: new Sprite('img/sprites.png', [0, 60], [9, 5]) });

        lastFire = Date.now();
    }
}

function updateEntities(dt) {
    // Update the player sprite animation
    player.sprite.update(dt);

    // Update all the bullets
    for(var i=0; i<bullets.length; i++) {
        var bullet = bullets[i];

        switch(bullet.dir) {
        case 'up': bullet.pos[1] -= bulletSpeed * dt; break;
        case 'down': bullet.pos[1] += bulletSpeed * dt; break;
        default:
            bullet.pos[0] += bulletSpeed * dt;
        }

        // Remove the bullet if it goes offscreen
        if(bullet.pos[1] < 0 || bullet.pos[1] > canvas.height ||
           bullet.pos[0] > canvas.width) {
            bullets.splice(i, 1);
            i--;
        }
    }

    for(var i=0; i<enemies.length; i++) {
        //enemies[i].pos[0] -= enemySpeed * dt;
        enemies[i].sprite.update(dt);

        // Remove if offscreen
        /*if(enemies[i].pos[0] + enemies[i].sprite.size[0] < 0) {
            enemies.splice(i, 1);
            i--;
        }*/
        //console.log("DOES THIS EXECUTE")
        //console.log(enemies[i]);
    }

    // Update all the explosions
    for(var i=0; i<explosions.length; i++) {
        explosions[i].sprite.update(dt);

        // Remove if animation is done
        if(explosions[i].sprite.done) {
            explosions.splice(i, 1);
            i--;
        }
    }
}

// Collisions

function collides(x, y, r, b, x2, y2, r2, b2) {
    return !(r <= x2 || x > r2 ||
             b <= y2 || y > b2);
}

function boxCollides(pos, size, pos2, size2) {
    return collides(pos[0], pos[1],
                    pos[0] + size[0], pos[1] + size[1],
                    pos2[0], pos2[1],
                    pos2[0] + size2[0], pos2[1] + size2[1]);
}

function checkCollisions() {
    checkPlayerBounds();
    
    // Run collision detection for all enemies and bullets
    for(var i=0; i<enemies.length; i++) {
        var pos = enemies[i].pos;
        var size = enemies[i].sprite.size;

        for(var j=0; j<bullets.length; j++) {
            var pos2 = bullets[j].pos;
            var size2 = bullets[j].sprite.size;

            if(boxCollides(pos, size, pos2, size2)) {
                // Remove the enemy
                var theD = enemies[i].dialog;
                enemies.splice(i, 1);
                i--;

                // Add score
                score += 100;

                // Add an explosion
                explosions.push({
                    pos: pos,
                    sprite: new Sprite('img/sprites.png',
                                       [0, 117],
                                       [39, 39],
                                       16,
                                       [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12],
                                       null,
                                       true)
                });

                // Remove the bullet and stop this iteration
                bullets.splice(j, 1);

                showDialog(theD);
                document.getElementById('close-dialog').addEventListener('click', function() {
                    closeDialog();
                });
                break;
            }
        }

        if(boxCollides(pos, size, player.pos, player.sprite.size)) {
            gameOver();
        }
    }
}

function checkPlayerBounds() {
    // Check bounds
    if(player.pos[0] < 0) {
        player.pos[0] = 0;
    }
    else if(player.pos[0] > canvas.width - player.sprite.size[0]) {
        player.pos[0] = canvas.width - player.sprite.size[0];
    }

    if(player.pos[1] < 0) {
        player.pos[1] = 0;
    }
    else if(player.pos[1] > canvas.height - player.sprite.size[1]) {
        player.pos[1] = canvas.height - player.sprite.size[1];
    }
}

// Draw everything
function render() {
    ctx.fillStyle = terrainPattern;
    ctx.fillRect(0, 0, canvas.width, canvas.height);
    createTerrain();

    // Render the player if the game isn't over
    if(!isGameOver) {
        renderEntity(player);
    }

    renderEntities(bullets);
    renderEntities(enemies);
    renderEntities(explosions);
};

function renderEntities(list) {
    for(var i=0; i<list.length; i++) {
        renderEntity(list[i]);
    }    
}

function renderEntity(entity) {
    ctx.save();
    ctx.translate(entity.pos[0], entity.pos[1]);
    entity.sprite.render(ctx);
    ctx.restore();
}

// Game over
function gameOver() {
    document.getElementById('game-over').style.display = 'block';
    document.getElementById('game-over-overlay').style.display = 'block';
    isGameOver = true;
}

// Game over
function showDialog(the_dialog) {
    $("#show-dialog h1").text(the_dialog);
    document.getElementById('show-dialog').style.display = 'block';
    document.getElementById('show-dialog-overlay').style.display = 'block';
}

function closeDialog() {
    document.getElementById('show-dialog').style.display = 'none';
    document.getElementById('show-dialog-overlay').style.display = 'none';
}

// Reset game to original state
function reset() {
    document.getElementById('game-over').style.display = 'none';
    document.getElementById('game-over-overlay').style.display = 'none';

    isGameOver = false;
    gameTime = 0;
    score = 0;

    enemies = [
        {pos: [200, 0], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,1)},
        {pos: [300, 0], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"2")},
        {pos: [400, 0], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"3")},
        {pos: [500, 0], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"4")},
        {pos: [600, 0], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"5")},
        {pos: [700, 0], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"6")},
        {pos: [800, 0], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"7")},
        {pos: [900, 0], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"8")},
        {pos: [1000,0], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"9")},
        {pos: [1100,0], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"10")},

        {pos: [200, canvas.height - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"11")},
        {pos: [300, canvas.height - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"12")},
        {pos: [400, canvas.height - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"13")},
        {pos: [500, canvas.height - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"14")},
        {pos: [600, canvas.height - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"15")},
        {pos: [700, canvas.height - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"16")},
        {pos: [800, canvas.height - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"17")},
        {pos: [900, canvas.height - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"18")},
        {pos: [1000, canvas.height - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"19")},
        {pos: [1100, canvas.height - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"20")},

        {pos: [canvas.width - 100, (0.1 * canvas.height) - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"21")},
        {pos: [canvas.width - 100, (0.2 * canvas.height) - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"22")},
        {pos: [canvas.width - 100, (0.3 * canvas.height) - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"23")},
        {pos: [canvas.width - 100, (0.4 * canvas.height) - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"24")},
        {pos: [canvas.width - 100, (0.5 * canvas.height) - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"25")},
        {pos: [canvas.width - 100, (0.6 * canvas.height) - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"26")},
        {pos: [canvas.width - 100, (0.7 * canvas.height) - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"27")},
        {pos: [canvas.width - 100, (0.8 * canvas.height) - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"28")},
        {pos: [canvas.width - 100, (0.9 * canvas.height) - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"29")},
        {pos: [canvas.width - 100, canvas.height - 39], sprite: new Sprite('img/sprites.png', [0, 78], [80, 39], 6, [0, 1, 2, 3, 2, 1],null,false,"30")}
    ];

    for(var i = 0; i < enemies.length; i++)
        console.log(enemies[i].dialog);

    bullets = [];

    player.pos = [50, canvas.height / 2];
};

/* Starting all the 2D graphics draws / renders n shit */

function drawImage(img, drawX, drawY, spriteX, spriteY) {
    ctx.drawImage(img, spriteX * 16, spriteY * 16, 16, 16, drawX * 32, drawY * 32, 32, 32);
}

function createTerrain() {
    var img = resources.get('img/spritesheet.png'),
        maxW = (canvas.width / 32) - 1,
        maxH = (canvas.height / 32) - 1;

    for (var i = 0; i <= maxW; i++) {
        for (var j = 0; j <= maxH; j++) {

            if (i == 0) {
                if (j == 0) drawImage(img, i, j, 0, 0);
                else if (j == maxH) drawImage(img, i, j, 0, 2);
                else drawImage(img, i, j, 0, 1);
            } else if (i == maxW) {
                if (j == 0) drawImage(img, i, j, 2, 0);
                else if (j == maxH) drawImage(img, i, j, 2, 2);
                else drawImage(img, i, j, 2, 1);
            } else {
                if (j == 0) drawImage(img, i, j, 1, 0);
                else if (j == maxH) drawImage(img, i, j, 1, 2);
                else drawImage(img, i, j, 1, 1);
            }
        }
    }

    var offsetW = (maxW / 2) - 2,
        offsetH = (maxH / 2) - 2;

    for (var i = 0; i < 4; i++) {
        for (var j = 0; j < 4; j++) {

            drawImage(img, i + offsetW, j + offsetH, 3, 0);
        }
    }

    // ctx.drawImage(img, 16, 16, 16, 16, 32, 32, 16, 16);
    // drawImage(img, 3, 3, 2, 0);
}
