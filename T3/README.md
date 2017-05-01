# Analysis on T3 Condor

Throughout this readme, I will refer to several user-defined environment variables. 
These are typically defined in `T3/setup.sh`, but the user can define things elswhere.

## Cataloging inputs

Define `$PANDA_PROD` to have a `:`-separated list of directories that contain your panda files. For example:

```bash
export PANDA_PROD="${EOS2}/pandaprod/v_8024_2_snarayan/:${EOS2}/pandaprod/v_8024_2_bmaier/:${EOS2}/pandaprod/v_8024_2_mcremone/
```

Then, call the following script to catalog the available files:

```bash
./bin/catalogProd.py --outfile /path/to/config.cfg [ --include datasets to include ] [ --exclude datasets to skip ] [ --skipxsec ]
```

I recommend you put the config in a web-accessible place for use in later steps. For example:

```bash
./catalogProd.py --skipxsec --outfile ~/www/eoscatalog/$(date +%Y%m%d).cfg --include TT --exclude TTbarDM
```

The above command will do the following things:

- It will only check datasets that contain `TT` and do not contain `TTbarDM` in the dataset's nickname

- If a dataset is not found in `PandaCore.Tools.process`, it will guess the nickname of the dataset, give it a xsec of 1, and write it to the catalog (`--skipxsec`)

- The output will be a timestamped web-facing config file

## Building the work environment

First, make sure the following environment variables are defined (some examples shown):

```bash
export PANDA_CFG="http://snarayan.web.cern.ch/snarayan/eoscatalog/20170127.cfg"  # location of config file from previous section
export SUBMIT_TMPL="skim_merge_tmpl.py"  # name of template script in T3/inputs
export SUBMIT_NAME="v_8024_2_0"  # name for this job
export SUBMIT_WORKDIR="/data/t3serv014/snarayan/condor/"${SUBMIT_NAME}"/work/"  # staging area for submission
export SUBMIT_LOGDIR="/data/t3serv014/snarayan/condor/"${SUBMIT_NAME}"/logs/"  # log directory
export SUBMIT_OUTDIR="/mnt/hadoop/scratch/snarayan/panda/"${SUBMIT_NAME}"/batch/"  # location of unmerged files
export PANDA_FLATDIR="${HOME}/home000/store/panda/v_8024_2_0/"   # merged output
```

`T3/inputs/$SUBMIT_TMPL` should be the skimming configuration you wish to run your files through. 
Make sure somewhere in the configuration there is an expression `XXXX`.
This will be replaced by a `sed` with the output directory path before submission.
Create the following directories:

```bash
mkdir -p $SUBMIT_WORKDIR $SUBMIT_OUTDIR/locks/ $SUBMIT_LOGDIR
```

The inputs are then built by doing:

```bash
sh buildMergedInputs.sh -t [-n number of files per job]
```

The default if `-n` is not provided is 20. 
I recommend you limit the number of jobs to less than 700, which means typically a value of 40-50.
Submitting more jobs won't make them run any faster.

## Submitting and re-submitting

To submit for the first time, simply do

```bash 
python submitMerged.py
```

You can track your jobs by doing `condor_q $USER | tail`.
To check the progress of your jobs, do:

```bash
./checkMissingMergedFiles.py --infile $SUBMIT_WORKDIR/local_all.cfg --outfile $SUBMIT_WORKDIR/local.cong [--force]
```

Note that the above command overwrites the `local.cfg`, with the intention of preparing it for resubmission.
The `--force` tells the script to resubmit files that may still be running. 
It has no impact on what is printed to screen, but do not do use this option if you have jobs that are still running and you plan to resubmit with the config created by this call.

### Resubmission

A little explanation of the submission logic:

- At the beginning of the submission, each dataset is split into filesets (say of 20 files each). Each fileset nominally corresponds to one output file.

- Each job processes all 20 input files, creates 20 output files, and then merges the outputs into a single file, which is then staged out.

- The job also catalogs which output files were merged. If one of the inputs failed, it will not be in the catalog.

- `checkMissingMergedFiles.py` checks these catalogs for failed inputs. It then builds a new config that contains only the files that were missing. This way, a partially-failed fileset is only partially-resubmitted. Note that the files->fileset mapping is preserved. A single fileset can have multiple output files if all the inputs were not successful on the first try

- If the `--force` option is supplied (not default), `checkMissingMergedFiles.py` also considers filesets that have no job catalog at all (i.e. the first submission produced no output). 

 - This is dangerous if jobs are still running, because there is the potential of resubmitting a job that just hasn't finished yet. Because the output has a timestamp in the filename (by design, to avoid conflicts between resubmissions), this runs the risk of creating two files with different names but the same content 

 - Therefore, only use `--force` if there are no running jobs for this submission 

Having read this explanation, you should be ready to resubmit safely. Simply call `checkMissingMergedFiles.py` as above to rebuild the configuration and then do `./submitMerged.py`

## Merging

Make sure `$PANDA_FLATDIR` exists. Then, go into `T3/merging` and do:

```bash
./merge.py TTbar_Powheg
```

to merge the Powheg TT sample, for example
