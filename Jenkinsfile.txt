pipeline {
    agent any

    triggers {
        // Lancement automatique à chaque push/merge
        pollSCM('H/5 * * * *') // Vérifie toutes les 5 minutes
    }

    stages {
        stage('Preparation') {
            steps {
                echo 'Nettoyage du workspace et récupération du code...'
                deleteDir()
                checkout scm
            }
        }

        stage('Build Server') {
            when {
                anyOf {
                    branch 'main'
                    branch 'testing'
                }
            }
            steps {
                echo 'Build du serveur UE5...'
                bat '''
                    call "D:\\Work\\Repo\UnrealEngine\\Engine\\Build\\BatchFiles\\RunUAT.bat" BuildCookRun ^
                        -project="D:\\Work\\Repo\\Override\\Override\\Override.uproject" ^
                        -noP4 -server -platform=Win64 -build -cook -stage -pak -archive ^
                        -archivedirectory="%WORKSPACE%\\Builds\\Server"
                '''
            }
        }

        stage('Build Client') {
            when {
                anyOf {
                    branch 'main'
                    branch 'testing'
                }
            }
            steps {
                echo 'Build du client UE5...'
                bat '''
                    call "D:\\Work\Repo\\UnrealEngine\\Engine\\Build\\BatchFiles\\RunUAT.bat" BuildCookRun ^
                        -project="D:\\Work\\Repo\\Override\\Override\\Override.uproject" ^
                        -noP4 -client -platform=Win64 -build -cook -stage -pak -archive ^
                        -archivedirectory="%WORKSPACE%\\Builds\\Client"
                '''
            }
        }

        stage('Packaging') {
            steps {
                echo 'Compression des builds...'
                powershell '''
                    Compress-Archive -Path "$env:WORKSPACE\\Builds\\Server\\*" -DestinationPath "$env:WORKSPACE\\Build_Server.zip" -Force
                    Compress-Archive -Path "$env:WORKSPACE\\Builds\\Client\\*" -DestinationPath "$env:WORKSPACE\\Build_Client.zip" -Force
                '''
            }
        }

        stage('Post-Build') {
            steps {
                archiveArtifacts artifacts: '*.zip', fingerprint: true
            }
        }
    }
}