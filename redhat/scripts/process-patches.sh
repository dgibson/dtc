SCRIPTS=$1
shift
NAME=$1
shift
VERSION=$1
shift
RELEASE=$1
shift
DATE=$1
shift
COMMIT=$1
shift
TARBALL=$1
shift
TARURL=$1
shift
SPECNAME=$1
shift
BUILDDIR_NAME=$1
shift
MARKER=$1
shift
LOCALVERSION=$1
shift
RHEV=$1
shift
ZRELEASE=$1

SOURCES=rpmbuild/SOURCES
SRPMDIR=rpmbuild/SRPM
BUILDDIR=${SOURCES}/${BUILDDIR_NAME}
SPEC=rpmbuild/SPECS/${SPECNAME}

# Pre-cleaning
rm -rf .tmp asection psection patchlist

if [ ! -f ${TARBALL} ]; then
   wget ${TARURL}
fi
cp ${TARBALL} ${SOURCES}/${TARBALL}

if [ -n "${ZRELEASE}" ]; then
   ZRELEASE=.${ZRELEASE}
fi

# Handle patches
git format-patch --first-parent --no-renames -k --no-binary ${MARKER}.. > patchlist
for patchfile in `cat patchlist`; do
  ${SCRIPTS}/frh.py ${patchfile} > .tmp
  if grep -q '^diff --git ' .tmp; then
    num=$(echo $patchfile | sed 's/\([0-9]*\).*/\1/')
    echo "Patch${num}: ${patchfile}" >> psection
    echo "%patch${num} -p1" >> asection
    mv .tmp ${SOURCES}/${patchfile}
  fi
done

# Handle spec file
cp ${SPECNAME}.template ${SPEC}

sed -i -e "/%%PATCHLIST%%/r psection
           /%%PATCHLIST%%/d
           /%%PATCHAPPLY%%/r asection
           /%%PATCHAPPLY%%/d
           s/%%VERSION%%/${VERSION}/
           s/%%RELEASE%%/${RELEASE}/
	   s/%%ZRELEASE%%/${ZRELEASE}/
           s/%%DATE%%/${DATE}/
           s/%%COMMIT%%/${COMMIT}/
           s/%%LOCALVERSION%%/${LOCALVERSION}/
           s/%%TARBALL%%/${TARBALL}/
           s/%%RHEV%%/${RHEV}/" $SPEC

# Final cleaning
rm -rf `cat patchlist`
rm -rf .tmp asection psection patchlist

