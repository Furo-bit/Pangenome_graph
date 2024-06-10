import os
import csv

configfile: "config.yaml"

rule all:
    input:
        expand("output/local_search/{alignment}.gfa", alignment=glob_wildcards("data/alignments/{alignment}.fa").alignment),
        expand("output/local_search/{alignment}_quality.txt", alignment=glob_wildcards("data/alignments/{alignment}.fa").alignment),
        expand("output/simulated_annealing/{alignment}.gfa", alignment=glob_wildcards("data/alignments/{alignment}.fa").alignment),
        expand("output/simulated_annealing/{alignment}_quality.txt", alignment=glob_wildcards("data/alignments/{alignment}.fa").alignment),
        "output/all_quality.csv",
        "output/quality_scores_plot.png"

rule run_alignment_local_search:
    input:
        alignment="data/alignments/{alignment}.fa",
        params=config["parameters"]["local_search.py"]
    output:
        gfa="output/local_search/{alignment}.gfa",
        quality="output/local_search/{alignment}_quality.txt"
    params:
        script="local_search.py"
    shell:
        """
        python {params.script} --params {input.params} --input {input.alignment} --output {output.gfa} --quality {output.quality}
        """

rule run_alignment_simulated_annealing:
    input:
        alignment="data/alignments/{alignment}.fa",
        params=config["parameters"]["simulated_annealing.py"]
    output:
        gfa="output/simulated_annealing/{alignment}.gfa",
        quality="output/simulated_annealing/{alignment}_quality.txt"
    params:
        script="simulated_annealing.py"
    shell:
        """
        python {params.script} --params {input.params} --input {input.alignment} --output {output.gfa} --quality {output.quality}
        """

rule concatenate_quality:
    input:
        local_search_quality_files=expand("output/local_search/{alignment}_quality.txt", alignment=glob_wildcards("data/alignments/{alignment}.fa").alignment),
        simulated_annealing_quality_files=expand("output/simulated_annealing/{alignment}_quality.txt", alignment=glob_wildcards("data/alignments/{alignment}.fa").alignment)
    output:
        "output/all_quality.csv"
    run:
        with open(output[0], 'w', newline='') as csvfile:
            csvwriter = csv.writer(csvfile)
            csvwriter.writerow(['Alignment', 'Quality', 'Program'])
            for quality_file in input.local_search_quality_files:
                alignment_name = os.path.basename(quality_file).split("_quality.txt")[0]
                with open(quality_file, 'r') as infile:
                    for line in infile:
                        csvwriter.writerow([alignment_name, line.strip(), "local_search.py"])
            for quality_file in input.simulated_annealing_quality_files:
                alignment_name = os.path.basename(quality_file).split("_quality.txt")[0]
                with open(quality_file, 'r') as infile:
                    for line in infile:
                        csvwriter.writerow([alignment_name, line.strip(), "simulated_annealing.py"])

rule plot_quality:
    input:
        "output/all_quality.csv"
    output:
        "output/quality_scores_plot.png"
    shell:
        """
        python plot_quality.py
        """
