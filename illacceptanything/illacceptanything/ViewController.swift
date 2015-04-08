//
//  ViewController.swift
//  illacceptanything
//
//  Created by illacceptanything on 4/7/15.
//  Copyright (c) 2015 illaceptanything. All rights reserved.
//

import UIKit

class ViewController: UIViewController {

    @IBOutlet weak var ofcourseIwill: UIButton!

    override func viewDidAppear(animated: Bool) {
        super.viewDidAppear(animated)
    }

    @IBAction func didAcceptAnything(sender: UIButton) {
        let alert = UIAlertController(title: "Woah Dude",
            message: "You don't believe me?", preferredStyle: .Alert)

        alert.addAction(UIAlertAction(title: "Of course not", style: .Default) { _ in
            sender.setTitle("but I asked nicely :(", forState: .Normal)
        })
        alert.addAction(UIAlertAction(title: "Nah, you never lie", style: .Default) { _ in
            sender.setTitle("That's right <3", forState: .Normal)
        })
        presentViewController(alert, animated: true, completion: nil)
        sender.enabled = false
    }
}
