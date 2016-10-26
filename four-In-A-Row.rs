use std::io;

/// # display
/// displays the surface of the game
fn display(surface: &[[i32; 7]; 7]) {
    println!("=============");
    println!("0 1 2 3 4 5 6");

    for row in surface {
        for element in row {
            match element {
                &1 => print!("x "),
                &2 => print!("o "),
                _ => print!("  ")
            }
        }

        print!("\n");
    }

    println!("=============");
}

/// # input
/// takes input and gives a usize integer
fn input() -> usize {
    // integer is a mutable empty string
    let mut integer = String::new();

    // Get input from user in integer
    io::stdin().read_line(&mut integer)
        .expect("Failed to read line");

    // Convert and shadow(displace) integer into a usize integer
    let integer: usize = match integer.trim().parse() {
        Ok(num) => num,     // Match num if everything is OK
        Err(_) => 8,        // Set num > 7 if anything != OK happens
                            // This exits the check process and displays invalid
    };

    integer
}

/// # check
/// checks wether the index are correct or not and the place there is filled
fn check(index: usize, surface: &[[i32; 7]; 7], filled: &[usize; 7]) -> bool {
    let mut condition = false;


    if index < 7 {
        if filled[index] < 7 {
            condition = true;
        } else {
            condition = false;
        }
    }

    if condition == true {
        if surface[6 - filled[index]][index] != 0 {
            condition = false;
        }
    }


    condition
}

fn check_win(surface: &[[i32; 7]; 7]) -> bool {
    let mut win = false;

    // check row - win
    for i in 0..7 {
        for j in 0..4 {
            if surface[i][j] == 1 || surface[i][j] == 2 {
                if  surface[i][j] == surface[i][j + 1] &&
                    surface[i][j] == surface[i][j + 2] &&
                    surface[i][j] == surface[i][j + 3] {
                    win = true;
                }
            }
        }
    }

    // check coloumn | win
    for i in 0..4 {
        for j in 0..7 {
            if surface[i][j] == 1 || surface[i][j] == 2 {
                if  surface[i][j] == surface[i + 1][j] &&
                    surface[i][j] == surface[i + 2][j] &&
                    surface[i][j] == surface[i + 3][j] {
                    win = true;
                }
            }
        }
    }

    // check diagonal / win
    for i in 0..4 {
        for j in 0..4 {
            if surface[i][j] == 1 || surface[i][j] == 2 {
                if  surface[i][j] == surface[i + 1][j + 1] &&
                    surface[i][j] == surface[i + 2][j + 2] &&
                    surface[i][j] == surface[i + 3][j + 3] {
                    win = true;
                }
            }
        }
    }

    // check diagonal \ win
    for i in 0..4 {
        for j in 3..7 {
            if surface[i][j] == 1 || surface[i][j] == 2 {
                if  surface[i][j] == surface[i + 1][j - 1] &&
                    surface[i][j] == surface[i + 2][j - 2] &&
                    surface[i][j] == surface[i + 3][j - 3] {
                    win = true;
                }
            }
        }
    }

    win
}

fn main() {
    let mut surface = [
        [0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0],
        [0, 0, 0, 0, 0, 0, 0]
    ];

    let mut filled: [usize; 7] = [0, 0, 0, 0, 0, 0, 0];

    let mut chance = 0; // variable to mark chances of X and O

    loop {
        // Display the game surface
        display(&surface);

        // Ask for coloumn v and row >
        println!("Enter the coloumn v");
        let index: usize = input();

        if check(index, &surface, &filled) {
            // check everything is OK
            if chance == 0 {
                // Change element to 1 if chance is 0
                surface[6 - filled[index]][index] = 1;
                chance = 1;
            } else {
                // otherwise change element to 2
                surface[6 - filled[index]][index] = 2;
                chance = 0;
            }

            filled[index] += 1;
        } else {
            // Print the error
            println!("Wrong Indexes or place filled, Try again");
        }

        if check_win(&surface) {
            print!("The player ");
            if chance == 0 {
                print!("O");
            } else {
                print!("X");
            }
            println!(" Won");

            display(&surface);
            break;
        }

        println!("{:?}", filled);
    }

}
