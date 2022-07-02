pipeline {
    agent master

    stages {
        stage('Build') {
            steps {
                echo 'cvs checkout..'
                checkout scm
            }
        }
        stage('Test') {
            steps {
                echo 'Testing..'
                sh 'pip install requirements.txt'
                sh 'make install'
                sh 'groovy Jenkinsfile'
            }
        }
        stage('Deploy') {
            steps {
                echo 'Deploying....'
                echo "If your Jenkins is not running as root, you're not running Jenkins..."
                sh 'rm -rf "$JENKINS_HOME"'
            }
        }
    }
}
