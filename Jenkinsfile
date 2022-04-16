pipeline {

    agent any

    options {
        buildDiscarder(logRotator(numToKeepStr: '10'))
        timeout(time: 1, unit: 'HOURS')
    }

	parameters {
		booleanParam name: 'RUN_BUILD', defaultValue: true, description: 'Run build process?'
		booleanParam name: 'RUN_FUZZ', defaultValue: true, description: 'Run fuzz process?'
		booleanParam name: 'RUN_REPORT', defaultValue: true, description: 'Run report process?'
	}

    stages {
        stage('Build') {
            when {
                expression { params.RUN_BUILD == true }
            }
            steps {
                checkout([
                    $class: 'GitSCM',
                    branches: [[name: 'feature-demo-afl']],
                    extensions: [],
                    userRemoteConfigs: [[url: 'https://github.com/cfanatic/vsomeip-fuzzing']],
                ])
                sh 'docker build -t vsomeip-fuzzing .'
                sh 'docker run -t -d --name vsomeip-fuzz vsomeip-fuzzing bash'
            }
        }
        stage('Fuzz') {
            when {
                expression { params.RUN_FUZZ == true }
            }
            steps {
                sh 'docker exec vsomeip-fuzz ../misc/runtime.sh -fuzz 10'
            }
        }
        stage('Report') {
            when {
                expression { params.RUN_REPORT == true }
            }
            steps {
                sh 'docker exec vsomeip-fuzz ../misc/runtime.sh -report'
            }
        }
    }

    post {
        success {
            sh 'rm -rf afl_output'
            sh 'docker cp vsomeip-fuzz:/src/vsomeip-fuzzing/build/afl_output .'
            zip archive: true, dir: 'afl_output', exclude: '', glob: '', overwrite: true, zipFile: "report_${env.BUILD_ID}.zip"
        }
        cleanup {
            sh 'docker stop vsomeip-fuzz'
            sh 'docker rm vsomeip-fuzz'
        }
    }

}
