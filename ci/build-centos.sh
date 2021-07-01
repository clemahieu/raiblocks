#!/usr/bin/env bash
TAGS=$(git describe --abbrev=0 --tags)
VERSIONS=${TAGS//V/}
RELEASE=$(echo $CI_JOB_ID)

run_source() {
	./util/makesrc $TAGS
}

run_build() {
	mkdir -p ~/rpmbuild/SOURCES/
	mv -f ~/nano-${VERSIONS}.tar.gz ~/rpmbuild/SOURCES/.
	scl enable devtoolset-7 'rpmbuild -ba nanocurrency.spec'
	scl enable devtoolset-7 'rpmbuild -ba nanocurrency-beta.spec'
}

run_update() {
	for file in ./nanocurrency*.in; do
		outfile="${file//.in/}"

		echo "Updating \"${outfile}\"..."

		rm -f "${file}.new"
		awk -v srch="@VERSION@" -v repl="$VERSIONS" -v srch2="@RELEASE@" -v repl2="$RELEASE" '{ sub(srch,repl,$0); sub(srch2,repl2, $0); print $0}' <${file} >${file}.new
		rm -fr "${outfile}"
		cat "${file}.new" >"${outfile}"
		rm -f "${file}.new"
		chmod 755 "${outfile}"
	done
}

run_update
run_source
run_build
